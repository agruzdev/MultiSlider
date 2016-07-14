package amaze.soft

import scala.concurrent.duration._

/**
 * Created by Alexey on 21.05.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
object Constants {
  val RESPONCE_GREETINGS = "HALO"
  val RESPONSE_SUCC = "SUCC"
  val RESPONSE_SUCK = "SUCK"
  val RESPONSE_ROOM_IS_FULL = "FULL"
  val RESPONSE_NAME_EXISTS  = "NAME"
  val RESPONSE_SUCK_IS_FULL = "SERVER_IS_FULL"
  val MESSAGE_EJECTED = "EJECTED"

  val FUTURE_TIMEOUT = 30 seconds

  val FRONTEND_NAME = "MultiSlider:Frontend"
  val BACKEND_NAME = "MultiSlider:Backend"

  val MESSAGE_ENCODING = "UTF-8"

  // Predefined messages in Update
  val UPDATE_JOINED  = "JOIN"
  val UPDATE_EJECTED = "EJECT"

  // Flags for ejected event
  val FLAG_EJECTED        = 1
  val FLAG_CLOSED_BY_HOST = 2
}
