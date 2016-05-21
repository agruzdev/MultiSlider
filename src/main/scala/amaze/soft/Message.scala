package amaze.soft

/**
 * Created by Alexey on 21.05.2016.
 *
 */
object Message {
  trait JsonMessage
  case class RegisterRoom(playerName: String, roomName: String) extends JsonMessage
  case class GetRooms() extends JsonMessage

}
