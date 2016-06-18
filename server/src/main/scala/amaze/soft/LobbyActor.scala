package amaze.soft

import akka.actor.{Actor, ActorRef}
import akka.io.Tcp
import akka.pattern.ask
import akka.util.{Timeout, ByteString}
import amaze.soft.Backend.CreateSession
import amaze.soft.Frontend.LobbyClosed
import amaze.soft.FrontendMessage._
import net.liftweb.json
import net.liftweb.json.{DefaultFormats, ShortTypeHints}
import org.slf4j.LoggerFactory

import scala.concurrent.Await
import scala.collection.JavaConversions._

/**
 * Created by Alexey on 20.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */

object LobbyActor
{
  // Internal structure describing room
  case class RoomInfo(name: String, host: PlayerInfo)
  // Simple room descriptor for clients
  case class RoomHeader(name: String, host: String)

  // States of lobby
  object State extends Enumeration {
    type State = Value
    val Virgin, Host, Joined, Zombie = Value
  }

  // Send message asking to add self to a lobby
  case class AddPlayer(player: PlayerInfo)
  // Send message asking to remove self from a lobby
  case class RemovePlayer(player: PlayerInfo)
  // Get this message if lobby was closed or you were ejected
  case class Ejected()
  // Broadcast Update message to all users except of updateSender
  case class UpdateBroadcast(updateSender: ActorRef, updateMsg: Update)

  val logger = LoggerFactory.getLogger(LobbyActor.getClass.getName)
}

class LobbyActor(
  val m_remote_user: ActorRef
) extends Actor {
  import LobbyActor.State._
  import LobbyActor._
  import Tcp._

  private implicit val formats = DefaultFormats.withHints(ShortTypeHints(List(
    classOf[CreateRoom], classOf[CloseRoom], classOf[GetRooms], classOf[JoinRoom], classOf[LeaveRoom],
    classOf[EjectPlayer], classOf[StartSession], classOf[SessionStarted], classOf[Update])))

  // Current lobby state
  var m_state = Virgin
  // Current room (valid only is Host or Joined)
  var m_room: RoomInfo = null
  // All players in the room
  var m_players: Map[ActorRef, PlayerInfo] = Map()
  // My player info
  var m_myself: PlayerInfo = null

  // Close lobby
  private def shutdown() = {
    logger.info("Disconnected")
    context stop self
  }

  // On created
  override def preStart() = {
    logger.info("Created handler for " + m_remote_user.path)
    //m_remote_user ! Tcp.Write(ByteString(Constants.RESPONCE_GREETINGS))
  }

  // Handle messages
  override def receive = {
    //---------------------------------------------------------------------
    // User messages got by TCP
    case Received(data) =>
      try {
        val msg = json.parse(data.decodeString(Constants.MESSAGE_ENCODING)).extract[JsonMessage]
        logger.info(msg.toString)
        msg match {
          //---------------------------------------------------------------------
          case CreateRoom(player, roomName) =>
            logger.info("Got a CreateRoom message!")
            m_myself = PlayerInfo(player, self)
            m_room = RoomInfo(roomName, m_myself)
            if ((m_state == Virgin) && Depot.registerLobby(roomName, m_room)) {
              m_players += self -> m_myself
              m_state = Host
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCC))
            } else {
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
            }
          //---------------------------------------------------------------------
          case CloseRoom() =>
            logger.info("Got a CloseRoom message!")
            if(m_state == Zombie) {
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCC))
            }
            if (m_state == Host) {
              Depot.unregisterLobby(m_room.name)
              m_state = Virgin
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCC))
            } else {
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
            }
          //---------------------------------------------------------------------
          case GetRooms() =>
            logger.info("Got a GetRooms message!")
            sender() ! Tcp.Write(ByteString(json.Serialization.write(Depot.getLobbies.map({ case (name, info) => RoomHeader(name, info.host.name) }).toList)))

          //---------------------------------------------------------------------
          case JoinRoom(player, roomName) =>
            logger.info("Got a JoinRoom message!")
            if (m_state == Virgin) {
              m_room = Depot.getLobbies.get(roomName)
              if (m_room != null) {
                m_myself = PlayerInfo(player, self)
                implicit val timeout = Timeout(Constants.FUTURE_TIMEOUT)
                if (Await.result(m_room.host.actor ? AddPlayer(m_myself), timeout.duration).asInstanceOf[Boolean]) {
                  m_state = Joined
                  sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCC))
                } else {
                  sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
                }
              }
            } else {
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
            }
          //---------------------------------------------------------------------
          case LeaveRoom() =>
            logger.info("Got a LeaveRoom message!")
            sender() ! Tcp.Write(ByteString({ if(leaveRoomImpl()) Constants.RESPONSE_SUCC else Constants.RESPONSE_SUCK }))
          //---------------------------------------------------------------------
          case EjectPlayer(playerName) =>
            logger.info("Got a EjectPlayer message!")
            if(m_state == Host) {
              val entry = m_players.find { case (actor, info) => info.name == playerName }
              if (entry.isDefined && ejectPlayerImpl(entry.get._2)) {
                sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCC))
              } else {
                sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
              }
            } else {
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
            }
          //---------------------------------------------------------------------
          case Update(_: String) =>
            logger.info("Got a UpdateBroadcast message!")
            if(m_state == Host || m_state == Joined) {
              m_room.host.actor ! UpdateBroadcast(self, msg.asInstanceOf[Update])
            }

          //---------------------------------------------------------------------
          case StartSession() =>
            logger.info("Got a StartSession message!")
            if(m_state == Host) {
              implicit val timeout = Timeout(Constants.FUTURE_TIMEOUT)
              val sessionId = Await.result(Depot.backend ? CreateSession(
                m_room.name, m_players.values.map(info => info.name).toList), timeout.duration).asInstanceOf[Int]
              if (sessionId >= 0) {
                m_players.values.foreach(info => { info.actor ! SessionStarted(Depot.getAddressBack, m_room.name, sessionId) })
              } else {
                sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
              }
            } else {
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
            }
          //---------------------------------------------------------------------
          case _ => logger.error("Unknown message!")
        }
      } catch {
        case err: Exception =>
          logger.error(err.getMessage, err.getCause)
          err.printStackTrace()
          sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
      }
    //---------------------------------------------------------------------
    // Service messages for handling connection
    case PeerClosed =>
      shutdown()

    case ErrorClosed(cause) =>
      logger.warn("Connection is closed; Reason = " + cause)
      shutdown()
    //---------------------------------------------------------------------
    // Internal messages between actors
    case AddPlayer(player) =>
      sender() ! addPlayerImpl(player)
      logger.info("All players = " + m_players)

    case RemovePlayer(player) =>
      sender() ! removePlayerImpl(player)
      logger.info("All players = " + m_players)

    case Ejected() =>
      if(m_state == Joined) {
        m_remote_user ! Tcp.Write(ByteString(Constants.MESSAGE_EJECTED))
        m_state = Virgin
      }

    case UpdateBroadcast(updateSender: ActorRef, updateMessage: Update) =>
      if(m_state == Host) {
        m_players.foreach{ case (playerActor, _) => if(playerActor != self) playerActor ! UpdateBroadcast(updateSender, updateMessage) }
      }
      if(self != updateSender) {
        m_remote_user ! Tcp.Write(ByteString( json.Serialization.write(updateMessage)))
      }

    case SessionStarted(address, name, sessionId) =>
      logger.info("Got a SessionStarted message!")
      // forward to remote client
      m_remote_user ! Tcp.Write(ByteString(json.Serialization.write(SessionStarted(address, name, sessionId))))
      leaveRoomImpl()
      m_state = Zombie

    //---------------------------------------------------------------------
    case unknown: Any =>
      logger.warn("Got unknown message! Msg = " + unknown.toString)
  }

  // On close
  override def postStop() = {
    m_state match {
      case Host =>
        m_players foreach {
          case (actor, player) => if (actor != self) ejectPlayerImpl(player)
        }
        Depot.unregisterLobby(m_room.name)
      case Joined => leaveRoomImpl()
      case Virgin => ()
      case Zombie =>
        if(m_room != null && m_room.host.actor == self) {
          Depot.unregisterLobby(m_room.name)
        }
    }
    Depot.frontend ! LobbyClosed()
    logger.info("Destroyed handler for " + m_remote_user.path)
  }

  /**
   * Method of a Client
   */
  private def leaveRoomImpl() : Boolean = {
    if(m_state == Zombie) {
      return true
    }
    if(m_state == Joined) {
      implicit val timeout = Timeout(Constants.FUTURE_TIMEOUT)
      if(Await.result(m_room.host.actor ? RemovePlayer(m_myself), timeout.duration).asInstanceOf[Boolean]) {
        m_state = Virgin
        return true
      }
    }
    false
  }

  /**
   * Method of a Host
   * Add a player to the current room
   */
  private def addPlayerImpl(player: PlayerInfo) : Boolean = {
    if(m_state == Host) {
      if(m_players.get(player.actor).isEmpty) {
        m_players += player.actor -> player
        logger.info("Player \"" + player.name + "\" joined")
        return true
      }
    }
    logger.error("Can't add a player! I'm not a host!")
    false
  }

  /**
   * Method of a Host
   * Remove a player from the current room
   */
  private def removePlayerImpl(player: PlayerInfo) : Boolean = {
    if(m_state == Host) {
      m_players -= player.actor
      logger.info("Player \"" + player.name + "\" left")
      return true
    }
    logger.error("Can't remove a player! I'm not a host!")
    false
  }

  /**
   * Method of a Host
   */
  private def ejectPlayerImpl(player: PlayerInfo) : Boolean = {
    if(m_state == Host && player.actor != self) {
      if(removePlayerImpl(player)) {
        player.actor ! Ejected()
        return true
      }
    }
    false
  }
}


