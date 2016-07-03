package amaze.soft

/**
 * Created by Alexey on 21.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */

object FrontendMessage {
  trait JsonMessage

  /**
   * Client sends the message in order to create a new room
   * @param playerName player name who creates the room, he will be host
   * @param roomName the new room's name
   */
  case class CreateRoom(playerName: String, roomName: String, playersLimit: Int) extends JsonMessage

  /**
   * Close a previously created room
   */
  case class CloseRoom() extends JsonMessage

  /**
   * Get a list of all created rooms
   */
  case class GetRooms() extends JsonMessage

  /**
   * Join a room
   * @param playerName player name
   * @param roomName room name to join
   */
  case class JoinRoom(playerName: String, roomName: String) extends JsonMessage

  /**
   * Leave currently joined room
   */
  case class LeaveRoom() extends JsonMessage

  /**
   * Eject player from my room
   * Can be send only by host
   */
  case class EjectPlayer(playerName: String) extends JsonMessage

  /**
   * Is sent by any user when he changed some settings
   */
  case class Update(data: String) extends JsonMessage

  /**
   * Start game session for all current players
   */
  case class StartSession() extends JsonMessage

  /**
   * Sent when session is created
   */
  case class SessionStarted(ip: String, port: Int, name: String, id: Int) extends JsonMessage
}
