package amaze.soft

import amaze.soft.LobbyActor.LobbyInfo
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 20.05.2016.
 *
 */
object Depot {
  private val logger = LoggerFactory.getLogger(Depot.getClass.getName)
  private val lock: Object = new Object
  private var lobbies: Map[String, LobbyInfo] = Map()

  def registerLobby(actorId: String, info: LobbyInfo) : Boolean = {
    var status = false
    logger.info("Register lobby \"" + info.roomName + "\" created by \"" + info.hostName + "\"")
    lock.synchronized {
      if (lobbies.get(actorId).isEmpty) {
        lobbies += actorId -> info
        status = true
      }
    }
    logger.info("All lobbies = " + lobbies)
    status
  }

  def unregisterLobby(actorId: String) = {
    logger.info("Remove lobby handled by " + actorId)
    lock.synchronized {
      lobbies -= actorId
    }
    logger.info("All lobbies = " + lobbies)
  }

  def getLobbies = lobbies
}
