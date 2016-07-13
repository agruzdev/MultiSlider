package amaze.soft

/**
 * Created by Alexey on 11.07.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */

// Simple room descriptor for clients
case class RoomInfo(name: String, var host: String, playersLimit: Int, var playersNumber: Int, var players: List[String])
{
  def noPlayers = RoomInfo(name, host, playersLimit, playersNumber, null)
}
