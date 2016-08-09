package controllers

import akka.actor.ActorSystem
import akka.util.Timeout
import akka.pattern.ask
import com.typesafe.config.ConfigFactory
import controllers.DogApi.{Identity, Hello}
import net.liftweb.json
import net.liftweb.json.{DefaultFormats, ShortTypeHints}
import play.api.mvc.{Action, Controller}

import scala.concurrent.Await
import scala.concurrent.duration._

/**
 * Created by Alexey on 08.08.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
object Application extends Controller {

  case class ServerInfo(name: String, version: String, online: Boolean)

  val TITLE = "MultiSlider Watchdog"

  implicit val timeout = Timeout(5 seconds)
  val ms_actorsSystem = ActorSystem("WatchDogActors", ConfigFactory.load("akka.conf"))
  val ms_remoteApi = Await.result(ms_actorsSystem.actorSelection("akka.tcp://MultiSliderActors@127.0.0.1:5150/user/MultiSlider:DogApi").resolveOne(), timeout.duration)

  private implicit val formats = DefaultFormats.withHints(ShortTypeHints(List(classOf[DogApi.Hello])))

  private def checkDefaultServer(): ServerInfo = {
    if(ms_remoteApi != null) {
      try {
        val response = Await.result(ms_remoteApi ? json.Serialization.write(Hello()), timeout.duration).asInstanceOf[String]
        if (response != null) {
          val identity = json.parse(response).extract[Identity]
          return ServerInfo(identity.name, identity.verMajor.toString + "." + identity.verMinor.toString, online = true)
        }
      }
      catch {
        case _ : Exception => ()
      }
    }
    ServerInfo("MultiSlider", "N/A", online=false)
  }

  def index = Action {
    Ok(views.html.servers.render(List(checkDefaultServer())))
  }

}

