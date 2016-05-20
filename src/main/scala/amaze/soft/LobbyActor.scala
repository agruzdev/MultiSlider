package amaze.soft

import akka.actor.{ActorRef, Actor}
import akka.io.Tcp
import akka.util.ByteString
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 20.05.2016.
 *
 */

object LobbyActor
{
  val logger = LoggerFactory.getLogger(classOf[LobbyActor])
}

class LobbyActor(
  val host: ActorRef
) extends Actor {
  import Tcp._
  LobbyActor.logger.info("Created hadler for " + host.path)
  host ! Tcp.Write(ByteString("Hello Client!\n"))
  host ! Tcp.SO.KeepAlive(true)
  def receive = {
    case Received(data) =>
      LobbyActor.logger.info(data.toString())
      sender() ! Tcp.Write(ByteString("OK\n"))
    case PeerClosed     =>
      LobbyActor.logger.info("Disconnected")
      context stop self
  }
}


