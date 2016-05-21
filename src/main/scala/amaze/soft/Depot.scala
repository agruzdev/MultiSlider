package amaze.soft

import amaze.soft.LobbyActor.RoomInfo
import org.slf4j.LoggerFactory

/**
 * Created by Alexey on 20.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
object Depot {
  private val logger = LoggerFactory.getLogger(Depot.getClass.getName)
  private val lock: Object = new Object
  private var lobbies: Map[String, RoomInfo] = Map()

  def registerLobby(roomName: String, info: RoomInfo) : Boolean = {
    var status = false
    logger.info("Register lobby \"" + roomName + "\" created by \"" + info.host.name + "\"")
    lock.synchronized {
      if (lobbies.get(roomName).isEmpty) {
        lobbies += roomName -> info
        status = true
      }
    }
    logger.info("All lobbies = " + lobbies)
    status
  }

  def unregisterLobby(roomName: String) = {
    logger.info("Remove lobby \"" + roomName + "\"")
    lock.synchronized {
      lobbies -= roomName
    }
    logger.info("All lobbies = " + lobbies)
  }

  def getLobbies = lobbies
}
