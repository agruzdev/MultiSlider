package amaze.soft

import akka.actor.ActorRef
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 20.05.2016.
 *
 */
object Depot {
  private val logger = LoggerFactory.getLogger(Depot.getClass.getName)
  val lock: Object = new Object
  var lobbies: Map[String, ActorRef] = Map()

  def registerLobby(name: String, actor: ActorRef) : Boolean = {
    logger.info("Register lobby \"" + name + "\"")
    lock.synchronized {
      if (lobbies.get(name).isDefined) {
        return false
      } else {
        lobbies += name -> actor
        return true
      }
    }
  }
}
