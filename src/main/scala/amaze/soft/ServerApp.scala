package amaze.soft

import java.net.InetSocketAddress

import akka.actor._
import akka.camel.CamelExtension
import akka.io.{IO, Tcp}
import org.apache.camel.component.mina2.Mina2Component


/**
 * Created by Alexey on 19.05.2016.
 *
 */

object ServerApp extends App {

  val system = ActorSystem("MultiSliderActors")
  val camel = CamelExtension(system)
  camel.context.addComponent("mina2", new Mina2Component(camel.context))
  val server = system.actorOf(Props[ServerApp])
  println("Actors system is created!")
}

class SimplisticHandler extends Actor {
  import Tcp._
  def receive = {
    case Received(data) => sender() ! println(data)
    case PeerClosed     => println("disconnected"); context stop self
  }
}

class ServerApp extends Actor {

  import Tcp._
  IO(Tcp)(ServerApp.system) ! Bind(self, new InetSocketAddress("localhost", 8800))

  def receive = {
    case b @ Bound(localAddress) => println("listening " + localAddress)

    case CommandFailed(_: Bind) => context stop self

    case c @ Connected(remote, local) =>
      val handler = context.actorOf(Props[SimplisticHandler])
      val connection = sender()
      connection ! Register(handler)
  }
}
