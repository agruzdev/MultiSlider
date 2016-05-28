package amaze.soft

/**
 * Created by Alexey on 23.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
object BackendMessage {
  trait JsonMessage

  //-------------------------------------------------------------------------------
  // Messages from client to server
  /**
   * Session message wrapper
   * @param sessionId id of destination session
   * @param data serialized message
   */
  case class Envelop(sessionId: Int, data: String) extends JsonMessage

  /**
   * Sent by client when it is ready to begin session
   */
  case class Ready(playerName: String) extends JsonMessage

  /**
   * Update current player private data
   */
  case class Update(playerName: String, timestamp: Long, data: String) extends  JsonMessage

  //-------------------------------------------------------------------------------
  // Messages from server to client

  /**
   * Indicates that all players are joined and session can be started
   */
  case class Start() extends JsonMessage

  /**
   * Universal response - send current session state
   */
  case class SessionState() extends JsonMessage
}