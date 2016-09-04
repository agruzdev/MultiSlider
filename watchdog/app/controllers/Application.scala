package controllers

import java.io.File

import akka.actor.ActorSystem
import akka.pattern.ask
import akka.util.Timeout
import com.typesafe.config.ConfigFactory
import controllers.DogApi.{Hello, Identity}
import net.liftweb.json
import net.liftweb.json.{DefaultFormats, ShortTypeHints}
import play.api.Logger
import play.api.mvc.{Action, Controller}

import scala.concurrent.Await
import scala.concurrent.duration._
import sys.process._

/**
 * Created by Alexey on 08.08.2016.
 * The MIT License (MIT)
 * Copyright (c) 2016 Alexey Gruzdev
 */
object Application extends Controller {

  case class ServerInfo(name: String, version: String, online: Boolean)

  val TITLE = "MultiSlider Watchdog"

  val ms_actorsSystem = ActorSystem("WatchDogActors", ConfigFactory.load("akka.conf"))
  private implicit val formats = DefaultFormats.withHints(ShortTypeHints(List(classOf[DogApi.Hello])))

  private def checkDefaultServer(): ServerInfo = {
    try {
      implicit val timeout = Timeout(5 seconds)
      val ms_remoteApi = Await.result(ms_actorsSystem.actorSelection("akka.tcp://MultiSliderActors@127.0.0.1:5150/user/MultiSlider:DogApi").resolveOne(), timeout.duration)

      val response = Await.result(ms_remoteApi ? json.Serialization.write(Hello()), timeout.duration).asInstanceOf[String]
      if (response != null) {
        val identity = json.parse(response).extract[Identity]
        return ServerInfo(identity.name, identity.verMajor.toString + "." + identity.verMinor.toString, online = true)
      }
    }
    catch {
      case e : Exception =>
        //e.printStackTrace()
    }
    ServerInfo("MultiSlider", "N/A", online=false)
  }

  def index = Action {
    Ok(views.html.servers.render(List(checkDefaultServer())))
  }

  def kill(id: Int) = Action {
    try {
      val stdout = "ps ax" #| "grep multislider" !!;
      Logger.debug(stdout)
      stdout.split("\n").foreach(println)
      val pattern = "([0-9]+).*multislider-[0-9]+\\.[0-9]+\\.jar.*\\s?".r
      val app = stdout.split("\n").foreach{
        line =>
          Logger.debug(line)
          val matchOpt = pattern.findFirstMatchIn(line)
          if(matchOpt.isDefined && matchOpt.get.groupCount > 0){
            val pid = matchOpt.get.group(1)
            Logger.debug(pid)
            ("kill " + pid).!
          }
      }
    }catch {
      case e : Exception =>
        Logger.error("Exception", e)
    }
    Redirect("/")
  }

  def run(id: Int) = Action {
    try {
      Seq("sh", "-c", "cd /home/wr/MultiSlider && python run_latest.py").!
    }catch {
      case e : Exception =>
        Logger.error("Exception", e)
    }
    Redirect("/")
  }

  def log(id: Int) = Action {
    Ok.sendFile(new File("/home/wr/MultiSlider/multislider-log.txt"))
  }


}

