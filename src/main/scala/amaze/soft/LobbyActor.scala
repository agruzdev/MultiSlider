package amaze.soft

import akka.actor.{Actor, ActorRef}
import akka.io.Tcp
import akka.util.ByteString
import net.liftweb.json
import net.liftweb.json.{DefaultFormats, ShortTypeHints}
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 20.05.2016.
 *
 */

object LobbyActor
{
  object State extends Enumeration {
    type State = Value
    val Virgin, Opened, Ready = Value
  }

  trait JsonMessage
  case class RegisterRoom(playerName: String, roomName: String) extends JsonMessage

  val logger = LoggerFactory.getLogger(LobbyActor.getClass.getName)
}

class LobbyActor(
  val host: ActorRef
) extends Actor {
  import LobbyActor.State._
  import LobbyActor._
  import Tcp._

  implicit val formats = DefaultFormats.withHints(ShortTypeHints(List(classOf[RegisterRoom])))
  var state = Virgin

  private def shutdown() = {
    logger.info("Disconnected")
    context stop self
  }

  override def preStart() = {
    logger.info("Created hadler for " + host.path)
    Depot.registerLobby("1", self)
    host ! Tcp.Write(ByteString("Hello Client!\n"))
  }

  override def receive = {
    case Received(data) =>
      try {
        val msg = json.parse(data.decodeString("UTF-8")).extract[JsonMessage]
        logger.info(msg.toString)
        msg match {
          case RegisterRoom(player, room) => logger.error("Register message!")
          case _ => logger.error("Unknown message!")
        }
        sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCC + "\n"))
      } catch {
        case err: Exception =>
          logger.warn("Failed to parse json message")
          sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK + "\n"))
      }
    case PeerClosed     =>
      shutdown()
    case ErrorClosed(cause) =>
      logger.warn("Connection is closed; Reason = " + cause)
      shutdown()
    case unknown: Any =>
      logger.warn("Got unknown message! Msg = " + unknown.toString)
  }

  override def postStop() = {
    logger.info("Destroyed handler for " + host.path)
  }
}


