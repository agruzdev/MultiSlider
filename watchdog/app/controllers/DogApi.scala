package controllers

/**
 * Created by Alexey on 08.08.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
object DogApi {
    trait JsonMessage

    case class Hello() extends JsonMessage

    case class Identity(name: String, verMajor: Int, verMinor: Int)
}