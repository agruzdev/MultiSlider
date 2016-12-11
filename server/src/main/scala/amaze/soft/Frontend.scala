package amaze.soft

import java.io.File
import java.net.InetSocketAddress

import akka.actor._
import akka.io.{IO, Tcp, Udp}
import akka.util.ByteString
import amaze.soft.FrontendMessage.{GetRooms, Greeting, JsonMessage}
import com.typesafe.config.ConfigFactory
import net.liftweb.json
import net.liftweb.json.{DefaultFormats, ShortTypeHints}
import org.slf4j.LoggerFactory

import scala.collection.JavaConversions._


/**
 * Created by Alexey on 19.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */

object Frontend extends App {
  case class ClientDisconnected()

  private val MAX_CLIENTS_NUMBER = 1024
  private var m_clientsCounter = 0
  private val m_counterLock: Object = new Object

  private val m_logger = LoggerFactory.getLogger(Frontend.getClass.getName)

  // defaults
  Depot.ip_frontend   = "localhost"
  Depot.ip_backend    = "localhost"
  Depot.port_frontend = 8800
  Depot.port_backend  = 8700

  if(args.isEmpty){
    println("Invalid arguments")
    println("Cmd arguments: <config path>")
    System.exit(1)
  }
  try {
    val configFile = new File(args(0))
    val config = ConfigFactory.parseFile(configFile)
    m_logger.info("Using config: " + configFile.getAbsolutePath)
    if (config.hasPath("multislider.frontend")) {
      val frontend = config.getConfig("multislider.frontend")
      Depot.ip_frontend   = frontend.getString("ip");
      Depot.port_frontend = frontend.getInt("port");
    }
    if (config.hasPath("multislider.backends")) {
      val backends = config.getConfigList("multislider.backends")
      if (!backends.isEmpty) {
        val defaultBackend = backends.get(0)
        if (defaultBackend.hasPath("ip") && defaultBackend.hasPath("port")) {
          Depot.ip_backend   = defaultBackend.getString("ip")
          Depot.port_backend = defaultBackend.getInt("port")
        }
      }
    }
  } catch { case _ : Exception  =>
    println("Failed to read config file")
    System.exit(1)
  }

  Depot.frontend = Depot.actorsSystem.actorOf(Props[Frontend], Constants.FRONTEND_NAME)
  Depot.backend  = Depot.actorsSystem.actorOf(Props(classOf[Backend], Depot.ip_backend, Depot.port_backend), Constants.BACKEND_NAME)

  Depot.dogapi = Depot.actorsSystem.actorOf(Props[DogApi], Constants.DOG_API_NAME)
}


class Frontend extends Actor {
  import Frontend._

  private implicit val formats = DefaultFormats.withHints(ShortTypeHints(List(classOf[GetRooms], classOf[Greeting])))

  m_logger.info("Frontend is created!")
  IO(Tcp)(Depot.actorsSystem) ! Tcp.Bind(self, new InetSocketAddress(Depot.ip_frontend, Depot.port_frontend))
  m_logger.info("Created TCP socket for " + Depot.ip_frontend + ":" + Depot.port_frontend)
  IO(Udp)(Depot.actorsSystem) ! Udp.Bind(self, new InetSocketAddress(Depot.ip_frontend, Depot.port_frontend))
  m_logger.info("Created UDP socket for " + Depot.ip_frontend + ":" + Depot.port_frontend)

  def receive = {
    case Tcp.Bound(localAddress) =>
      m_logger.info("Listening to " + localAddress)

    case Tcp.CommandFailed(_: Tcp.Bind) =>
      m_logger.info("Fail"); context stop self

    case Tcp.Connected(remote, local) =>
      m_logger.info("remote = " + remote)
      m_logger.info("local  = " + local)

      var status = false
      m_counterLock.synchronized {
        if (m_clientsCounter < MAX_CLIENTS_NUMBER) {
          m_clientsCounter += 1
          status = true
        }
      }
      val client = sender()
      if(status) {
        m_logger.info("Clients number = " + m_clientsCounter)
        val handler = context.actorOf(Props(classOf[Client]))
        client ! Tcp.Register(handler)
        client ! Tcp.Write(ByteString(json.Serialization.write(
          Greeting(Constants.DEFAULT_NAME, Constants.VERSION_MAJOR, Constants.VERSION_MINOR, Constants.VERSION_REVISION))))
      } else {
        client ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK_IS_FULL))
        client ! Tcp.Close
      }


    case ClientDisconnected() =>
      m_counterLock.synchronized {
        m_clientsCounter = math.max(0, m_clientsCounter - 1)
      }
      m_logger.info("Clients number = " + m_clientsCounter)


    case Udp.Bound(local) =>
      m_logger.info("UDP socket is bound")

    case Udp.Received(data, remote) =>
      m_logger.info("Got a datagram")
      m_logger.info("Datagram = " + data.decodeString(Constants.MESSAGE_ENCODING))
      try {
        val msg = json.parse(data.decodeString(Constants.MESSAGE_ENCODING)).extract[JsonMessage]
        m_logger.info(msg.toString)
        msg match {
          case GetRooms() =>
            m_logger.info("Got a GetRooms message!")
            sender() ! Udp.Send(ByteString(json.Serialization.write(Depot.getLobbies.map({
              case (_, lobby) => lobby.info.noPlayers
            }).toList)), remote)

          case _ =>
            m_logger.error("Unknown message!")
        }
      } catch {
        case err: Exception =>
          m_logger.error(err.getMessage, err.getCause)
          err.printStackTrace()
          sender() ! Tcp.Write(ByteString(Constants.RESPONSE_SUCK))
      }

    case Udp.Unbind  =>
      sender() ! Udp.Unbind
  }

}
