package amaze.soft

import java.net.InetSocketAddress

import akka.actor._
import akka.io.{IO, Tcp}
import akka.util.ByteString
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 19.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */

object Frontend extends App {
  case class LobbyClosed()

  private val MAX_LOBBIES_NUMBER = 256
  private var m_lobbyActorsCounter = 0
  private val m_counterLock: Object = new Object

  private val m_logger = LoggerFactory.getLogger(Frontend.getClass.getName)

  // Cmd arguments: <ip> <frontend port> <backend port>
  Depot.ip_address    = if(args.length > 0) args(0) else "localhost"
  Depot.port_frontend = if(args.length > 1) args(1).toInt else 8800
  Depot.port_backend  = if(args.length > 2) args(2).toInt else 8700

  Depot.frontend = Depot.actorsSystem.actorOf(Props[Frontend], Constants.FRONTEND_NAME)
  Depot.backend  = Depot.actorsSystem.actorOf(Props(classOf[Backend], Depot.ip_address, Depot.port_backend), Constants.BACKEND_NAME)
}


class Frontend extends Actor {
  import Frontend._
  import Tcp._

  m_logger.info("Frontend is created!")
  IO(Tcp)(Depot.actorsSystem) ! Bind(self, new InetSocketAddress(Depot.ip_address, Depot.port_frontend))
  m_logger.info("Created TCP socket for " + Depot.ip_address + ":" + Depot.port_frontend)

  def receive = {
    case b @ Bound(localAddress) =>
      m_logger.info("Listening to " + localAddress)

    case CommandFailed(_: Bind) =>
      m_logger.info("Fail"); context stop self

    case c @ Connected(remote, local) =>
      m_logger.info("remote = " + remote)
      m_logger.info("local  = " + local)
      var status = false
      m_counterLock.synchronized {
        if (m_lobbyActorsCounter < MAX_LOBBIES_NUMBER) {
          m_lobbyActorsCounter += 1
          status = true
        }
      }
      val client = sender()
      if(status) {
        m_logger.info("Lobbies number = " + m_lobbyActorsCounter)
        val handler = context.actorOf(Props(classOf[LobbyActor], client))
        client ! Register(handler)
        client ! Tcp.Write(ByteString(Constants.RESPONSE_SUCC))
      } else {
        client ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK_IS_FULL))
        client ! Tcp.Close
      }

    case LobbyClosed() =>
      m_counterLock.synchronized {
        m_lobbyActorsCounter = math.max(0, m_lobbyActorsCounter - 1)
      }
      m_logger.info("Lobbies number = " + m_lobbyActorsCounter)
  }

}
