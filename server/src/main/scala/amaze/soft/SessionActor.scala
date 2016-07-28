package amaze.soft

import java.net.InetSocketAddress

import akka.actor.{Actor, ActorRef}
import akka.io.Udp
import akka.util.ByteString
import amaze.soft.Backend.CloseSession
import amaze.soft.BackendMessage._
import akka.actor.Cancellable
import net.liftweb.json
import net.liftweb.json.{DefaultFormats, ShortTypeHints}
import org.slf4j.LoggerFactory
import scala.concurrent.duration._
import scala.language.postfixOps

object SessionActor
{
  val KEEP_ALIVE_INTERVAL = 1000 millisecond
  val KEEP_ALIVE_LIMIT = 10 * KEEP_ALIVE_INTERVAL

  /**
   * Client data envelop with back address
   */
  case class SignedEnvelop(socket: ActorRef, address: InetSocketAddress, data: String)

  /**
   * Dynamic information for each player
   */
  case class PlayerStatistics(address: InetSocketAddress, updateTimestamp: Long, privateData: String, var lastPingTime: Long)

  /**
   * Data broadcasted for each player
   */
  case class PlayerData(name: String, data: String)

  object State extends Enumeration
  {
    type State = Value
    val Waiting, Running, Zombie = Value
  }
}

/**
 * Created by Alexey on 22.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
class SessionActor(m_id: Int, m_name: String, players: List[String]) extends Actor{
  import SessionActor.State._
  import SessionActor._

  private val m_logger = LoggerFactory.getLogger(SessionActor.getClass.getName)
  m_logger.info("Session \"" + m_name + "\" is created!")

  val m_stats: scala.collection.mutable.HashMap[String, PlayerStatistics] =
    scala.collection.mutable.HashMap(players.map(name => {name -> null}).toSeq:_*)

  var m_shared_stats = PlayerStatistics(null, 0, "", 0)

  var m_state = Waiting

  var m_lastPing = System.currentTimeMillis()

  var m_keepAliveTask: Cancellable = null

  m_logger.info(players.toString())

  private implicit val formats = DefaultFormats.withHints(ShortTypeHints(List(
    classOf[Ready], classOf[Start], classOf[Update], classOf[SessionState], classOf[Quit], classOf[RequestSync], classOf[Sync], classOf[KeepAlive])))

  private def gatherSessionData(): String = {
    json.Serialization write (m_stats.map{case (name, stats) => PlayerData(name, if(stats != null) stats.privateData else "")} toList)
  }

  private def checkConnection() = {
    val currTime = System.currentTimeMillis()
    m_stats foreach { case (name, player) =>
      if(player != null && (currTime > player.lastPingTime) && (currTime - player.lastPingTime > KEEP_ALIVE_LIMIT.toMillis)){
        m_logger.info("Connection with the player \"" + name + "\" is lost!")
        quitImpl(name)
      }
    }
  }

  private def quitImpl(playerName: String) = {
    val playerStats = m_stats.get(playerName)
    if (playerStats.isDefined && playerStats.get != null) {
      m_stats.update(playerName, null)
      m_logger.info("\"" + playerName + "\" quits session \"" + m_name + "\"")
      if (m_stats.values.forall(_ == null)) {
        m_logger.info("All players left session \"" + m_name + "\". Exiting.")
        m_keepAliveTask.cancel()
        Depot.backend ! CloseSession(m_id)
        context stop self
      }
    }
  }

  override def receive = {
    case SignedEnvelop(socket, address, data) =>
      m_logger.info("Got an Envelop message")
      try {
        val msg = json.parse(data).extract[JsonMessage]
        m_logger.info(msg.toString)
        m_state match {
          //--------------------------------------------------------
          case Waiting =>
            msg match {
              case Ready(playerName) =>
                m_logger.info("Got a Ready message")
                if (m_stats.contains(playerName)) {
                  m_stats.update(playerName, PlayerStatistics(address, 0, "", System.currentTimeMillis()))
                  // if no nulls then all players have joined
                  if(!m_stats.values.exists(_ == null)){
                    m_state = Running
                    m_stats.values.foreach({stat => socket ! Udp.Send(ByteString(json.Serialization.write(Start())), stat.address)})
                    m_keepAliveTask = Depot.actorsSystem.scheduler.schedule(KEEP_ALIVE_INTERVAL, KEEP_ALIVE_INTERVAL) {
                      checkConnection()
                      m_stats.values.foreach({player => if(player != null) socket ! Udp.Send(ByteString(json.Serialization.write(KeepAlive(null))), player.address)})
                    } (Depot.actorsSystem.dispatcher)
                  } else {
                    socket ! Udp.Send(ByteString(Constants.RESPONSE_SUCC), address)
                  }
                }
                else {
                  socket ! Udp.Send(ByteString(Constants.RESPONSE_SUCK), address)
                }

              case KeepAlive(playerName) =>
                val player = m_stats.get(playerName)
                if(player.isDefined){
                  player.get.lastPingTime = System.currentTimeMillis()
                }

              case _ =>
                m_logger.error("[Waiting] Unexpected message")
            }
          //--------------------------------------------------------
          case Running =>
              msg match {
                case Ready(playerName) =>
                  () // Ignore

                case Update(playerName, timestamp, forceBroadcast, privateData, sharedData) =>
                  m_logger.info("Got a Update message")
                  val playerStats = m_stats.get(playerName)
                  if(playerStats.isDefined){
                    if(playerStats.get != null && playerStats.get.updateTimestamp < timestamp) {
                      m_stats.update(playerName, PlayerStatistics(playerStats.get.address, timestamp, privateData, System.currentTimeMillis()))
                      if (sharedData != null && sharedData.nonEmpty && m_shared_stats.updateTimestamp < timestamp) {
                        m_shared_stats = PlayerStatistics(null, timestamp, sharedData, 0)
                      }
                      if(forceBroadcast) {
                        m_stats.values.foreach( stat => socket ! Udp.Send(ByteString(json.Serialization.write(SessionState(gatherSessionData(), m_shared_stats.privateData))), stat.address))
                      }else {
                        socket ! Udp.Send(ByteString(json.Serialization.write(SessionState(gatherSessionData(), m_shared_stats.privateData))), address)
                      }
                    } else {
                      m_logger.info("[Running] Got outdated Update message")
                    }
                  }

                case KeepAlive(playerName) =>
                  val player = m_stats.get(playerName)
                  if(player.isDefined){
                    player.get.lastPingTime = System.currentTimeMillis()
                  }

                case RequestSync(_, delay, syncId) =>
                  m_logger.info("Got a RequestSync message")
                  Depot.actorsSystem.scheduler.scheduleOnce(delay milliseconds) {
                    m_stats.values.foreach( player => if(player != null) socket ! Udp.Send(ByteString(json.Serialization.write(Sync(syncId))), player.address))
                  } (Depot.actorsSystem.dispatcher)

                case Quit(playerName) =>
                  m_logger.info("Got a Quit message")
                  quitImpl(playerName)

                case _ => m_logger.error("[Running] Unexpected message")
              }
          //--------------------------------------------------------
          case Zombie =>
            msg match {
              case _ => m_logger.error("[Running] Unexpected message")
            }
        }
      }
      catch {
        case err: Exception =>
          m_logger.error(err.getMessage, err.getCause)
          err.printStackTrace()
          socket ! Udp.Send(ByteString(Constants.RESPONSE_SUCK), address)
      }

    case _ =>
      m_logger.error("SessionActor: Unknown message!")
  }
}
