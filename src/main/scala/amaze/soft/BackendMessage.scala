package amaze.soft

/**
 * Created by Alexey on 23.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
object BackendMessage {
  trait JsonMessage

  /**
   * Session message wrapper
   * @param sessionId id of destination session
   * @param data serialized message
   */
  case class Envelop(sessionId: Int, data: String) extends JsonMessage

  /**
   * Sent by client to join session
   */
  case class Join(playerName: String) extends JsonMessage
}
