package amaze.soft

import java.util

import akka.actor.{ActorRef, ActorSystem}
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
  private val lobbies: util.TreeMap[String, RoomInfo] = new util.TreeMap()

  val actorsSystem = ActorSystem("MultiSliderActors")
  logger.info("Actors system is created!")

  var frontend: ActorRef = null
  var backend:  ActorRef = null

  var ip_address = ""
  var port_frontend = 0
  var port_backend = 0

  def registerLobby(roomName: String, info: RoomInfo) : Boolean = {
    var status = false
    logger.info("Register lobby \"" + roomName + "\" created by \"" + info.host.name + "\"")
    lock.synchronized {
      if (!lobbies.containsKey(roomName)) {
        lobbies.put(roomName, info)
        status = true
      }
    }
    logger.info("All lobbies = " + lobbies)
    status
  }

  def unregisterLobby(roomName: String) = {
    logger.info("Remove lobby \"" + roomName + "\"")
    lock.synchronized {
      lobbies.remove(roomName)
    }
    logger.info("All lobbies = " + lobbies)
  }

  def getLobbies = lobbies


  def getAddressFront = {
    ip_address + ":" + port_frontend.toString
  }

  def getAddressBack = {
    ip_address + ":" + port_backend.toString
  }
}
