package amaze.soft

import java.net.InetSocketAddress

import akka.actor._
import akka.io.{IO, Tcp}
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 19.05.2016.
 *
 */

object ServerApp extends App {
  private val logger = LoggerFactory.getLogger(ServerApp.getClass.getName)
  val actorsSystem = ActorSystem("MultiSliderActors")
  logger.info("Actors system is created!")
  val server = actorsSystem.actorOf(Props[ServerApp], ServerApp.getClass.getName)
  logger.info("Server is created!")
}


class ServerApp extends Actor {
  import Tcp._
  IO(Tcp)(ServerApp.actorsSystem) ! Bind(self, new InetSocketAddress("localhost", 8800))

  def receive = {
    case b @ Bound(localAddress) =>
      ServerApp.logger.info("Listening to " + localAddress)

    case CommandFailed(_: Bind) =>
      ServerApp.logger.info("Fail"); context stop self

    case c @ Connected(remote, local) =>
      ServerApp.logger.info("remote = " + remote)
      ServerApp.logger.info("local  = " + local)
      val _connection = sender()
      val _handler = context.actorOf(Props(classOf[LobbyActor], _connection))
      _connection ! Register(_handler)
  }
}
