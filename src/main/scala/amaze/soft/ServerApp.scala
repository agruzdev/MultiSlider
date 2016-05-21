package amaze.soft

import java.net.InetSocketAddress

import akka.actor._
import akka.io.{IO, Tcp}
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 19.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */

object ServerApp extends App {
  private val logger = LoggerFactory.getLogger(ServerApp.getClass.getName)
  val actorsSystem = ActorSystem("MultiSliderActors")
  logger.info("Actors system is created!")
  val server = actorsSystem.actorOf(Props[ServerApp], ServerApp.getClass.getName)
  logger.info("Server is created!")

  val m_ip   = if(args.length > 1) args(0) else "localhost"
  val m_port = if(args.length > 2) args(1).toInt else 8800
}


class ServerApp extends Actor {
  import ServerApp._
  import Tcp._

  logger.info("Create socket for " + m_ip + ":" + m_port)
  IO(Tcp)(ServerApp.actorsSystem) ! Bind(self, new InetSocketAddress(m_ip, m_port))

  def receive = {
    case b @ Bound(localAddress) =>
      logger.info("Listening to " + localAddress)

    case CommandFailed(_: Bind) =>
      logger.info("Fail"); context stop self

    case c @ Connected(remote, local) =>
      logger.info("remote = " + remote)
      logger.info("local  = " + local)
      val _connection = sender()
      val _handler = context.actorOf(Props(classOf[LobbyActor], _connection))
      _connection ! Register(_handler)
  }
}
