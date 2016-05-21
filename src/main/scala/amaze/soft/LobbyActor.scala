package amaze.soft

import akka.actor.{Actor, ActorRef}
import akka.io.Tcp
import akka.util.ByteString
import amaze.soft.Message._
import net.liftweb.json
import net.liftweb.json.{DefaultFormats, ShortTypeHints}
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 20.05.2016.
 *
 */

object LobbyActor
{
  case class LobbyInfo(hostName: String, roomName: String)

  object State extends Enumeration {
    type State = Value
    val Virgin, Opened, Ready = Value
  }

  val logger = LoggerFactory.getLogger(LobbyActor.getClass.getName)
}

class LobbyActor(
  val host: ActorRef
) extends Actor {
  import LobbyActor.State._
  import LobbyActor._
  import Tcp._

  implicit val formats = DefaultFormats.withHints(ShortTypeHints(List(classOf[RegisterRoom], classOf[GetRooms])))
  var state = Virgin
  val selfId = self.toString()

  private def shutdown() = {
    logger.info("Disconnected")
    context stop self
  }

  override def preStart() = {
    logger.info("Created handler for " + host.path)
    host ! Tcp.Write(ByteString(Constants.RESPONCE_GREETINGS))
  }

  override def receive = {
    case Received(data) =>
      try {
        val msg = json.parse(data.decodeString("UTF-8")).extract[JsonMessage]
        logger.info(msg.toString)
        msg match {
          case RegisterRoom(player, room) =>
            logger.info("Got a RegisterRoom message!")
            if(!Depot.registerLobby(selfId, LobbyInfo(player, room))){
              logger.warn("Room is already created!")
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
            } else {
              sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCC))
            }
          case GetRooms() =>
            logger.info("Got a GetRooms message!")
            sender() ! Tcp.Write(ByteString(json.Serialization.write(Depot.getLobbies)))
          case _ => logger.error("Unknown message!")
        }
      } catch {
        case err: Exception =>
          logger.error(err.getMessage, err.getCause)
          err.printStackTrace()
          sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
      }
    case PeerClosed =>
      shutdown()
    case ErrorClosed(cause) =>
      logger.warn("Connection is closed; Reason = " + cause)
      shutdown()
    case unknown: Any =>
      logger.warn("Got unknown message! Msg = " + unknown.toString)
  }

  override def postStop() = {
    Depot.unregisterLobby(selfId)
    logger.info("Destroyed handler for " + host.path)
  }
}


