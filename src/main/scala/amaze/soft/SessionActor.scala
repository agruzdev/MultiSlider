package amaze.soft

import java.net.InetSocketAddress

import akka.actor.{Actor, ActorRef}
import akka.io.Udp
import akka.util.ByteString
import org.slf4j.LoggerFactory

object SessionActor
{

  /**
   * Client data envelop with back address
   */
  case class SignedEnvelop(socket: ActorRef, address: InetSocketAddress, data: String)
}

/**
 * Created by Alexey on 22.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
class SessionActor extends Actor{
  import SessionActor._

  private val m_logger = LoggerFactory.getLogger(Frontend.getClass.getName)
  m_logger.info("Session Actor is created!")

  override def receive = {
    case SignedEnvelop(socket, address, data) =>
      m_logger.info("Got an Envelop message")
      socket ! Udp.Send(ByteString("Thank you!"), address)

    case _ =>
      m_logger.error("SessionActor: not implemented!")
  }
}
