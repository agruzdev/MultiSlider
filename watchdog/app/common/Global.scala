package common

import play.api.{Application, GlobalSettings, Logger}

/**
 * Created by Alexey on 10.08.2015.
 * Global settings
 */
object Global extends GlobalSettings {

  override def onStart(app: Application) {
    Logger.info("Application has started")
  }

  override def onStop(app: Application) {
    Logger.info("Application shutdown...")
  }

}
