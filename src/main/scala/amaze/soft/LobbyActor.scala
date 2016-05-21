package amaze.soft

import akka.actor.{Actor, ActorRef}
import akka.io.Tcp
import akka.util.ByteString
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 20.05.2016.
 *
 */

object LobbyActor
{
  val logger = LoggerFactory.getLogger(LobbyActor.getClass.getName)
}

class LobbyActor(
  val host: ActorRef
) extends Actor {
  import LobbyActor._
  import Tcp._

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
      logger.info(data.toString())
      sender() ! Tcp.Write(ByteString("OK\n"))
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


