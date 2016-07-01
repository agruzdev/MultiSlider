package amaze.soft

import java.net.InetSocketAddress

import akka.actor.{Actor, ActorRef}
import akka.io.Udp
import akka.util.ByteString
import amaze.soft.Backend.CloseSession
import amaze.soft.BackendMessage._
import net.liftweb.json
import net.liftweb.json.{DefaultFormats, ShortTypeHints}
import org.slf4j.LoggerFactory
import scala.concurrent.duration._

object SessionActor
{
  /**
   * Client data envelop with back address
   */
  case class SignedEnvelop(socket: ActorRef, address: InetSocketAddress, data: String)

  /**
   * Dynamic information for each player
   */
  case class PlayerStatistics(address: InetSocketAddress, updateTimestamp: Long, privateData: String)

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

  private val m_logger = LoggerFactory.getLogger(Frontend.getClass.getName)
  m_logger.info("Session \"" + m_name + "\" is created!")

  val m_stats: scala.collection.mutable.HashMap[String, PlayerStatistics] =
    scala.collection.mutable.HashMap(players.map(name => {name -> null}).toSeq:_*)
  m_logger.info(players.toString())

  var m_state = Waiting

  private implicit val formats = DefaultFormats.withHints(ShortTypeHints(List(
    classOf[Ready], classOf[Start], classOf[Update], classOf[SessionState], classOf[Quit], classOf[RequestSync], classOf[Sync])))

  private def gatherSessionData(): String = {
    json.Serialization write (m_stats.map{case (name, stats) => PlayerData(name, if(stats != null) stats.privateData else "")} toList)
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
                  m_stats.update(playerName, PlayerStatistics(address, 0, ""))
                  // if no nulls then all players have joined
                  if(!m_stats.values.exists(_ == null)){
                    m_state = Running
                    m_stats.values.foreach({stat => socket ! Udp.Send(ByteString(json.Serialization.write(Start())), stat.address)})
                  } else {
                    socket ! Udp.Send(ByteString(Constants.RESPONSE_SUCC), address)
                  }
                }
                else {
                  socket ! Udp.Send(ByteString(Constants.RESPONSE_SUCK), address)
                }
              case _ =>
                m_logger.error("[Waiting] Unexpected message")
            }
          //--------------------------------------------------------
          case Running =>
              msg match {
                case Ready(playerName) =>
                  () // Ignore

                case Update(playerName, timestamp, privateData) =>
                  m_logger.info("Got a Update message")
                  val playerStats = m_stats.get(playerName)
                  if(playerStats.isDefined){
                    if(playerStats.get != null && playerStats.get.updateTimestamp < timestamp) {
                      m_stats.update(playerName, PlayerStatistics(playerStats.get.address, timestamp, privateData))
                      socket ! Udp.Send(ByteString(json.Serialization.write(SessionState(gatherSessionData()))), address)
                    } else {
                      m_logger.info("[Running] Got outdated Update message")
                    }
                  }

                case RequestSync(_, delay, syncId) =>
                  m_logger.info("Got a RequestSync message")
                  Depot.actorsSystem.scheduler.scheduleOnce(delay milliseconds) {
                    m_stats.values.foreach( stat => socket ! Udp.Send(ByteString(json.Serialization.write(Sync(syncId))), stat.address))
                  } (Depot.actorsSystem.dispatcher)

                case Quit(playerName) =>
                  m_logger.info("Got a Quit message")
                  val playerStats = m_stats.get(playerName)
                  if(playerStats.isDefined && playerStats.get != null){
                    m_stats.update(playerName, null)
                    m_logger.info("\"" + playerName + "\" quits session \"" + m_name + "\"")
                    if(m_stats.values.forall(_ == null)){
                      m_logger.info("All players left session \"" + m_name + "\". Exiting.")
                      Depot.backend ! CloseSession(m_id)
                      context stop self
                    }
                  }

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
