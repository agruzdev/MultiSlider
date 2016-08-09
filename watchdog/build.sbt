name := "Watchdog"

version := "1.0"

lazy val `watchdog` = (project in file(".")).enablePlugins(PlayScala)

scalaVersion := "2.11.1"

libraryDependencies ++= Seq( jdbc , anorm , cache , ws )

// http://mvnrepository.com/artifact/net.liftweb/lift-json_2.11
libraryDependencies += "net.liftweb" % "lift-json_2.11" % "3.0-M8"

// https://mvnrepository.com/artifact/com.typesafe.akka/akka-remote_2.11
libraryDependencies += "com.typesafe.akka" % "akka-remote_2.11" % "2.3.6"

unmanagedResourceDirectories in Test <+=  baseDirectory ( _ /"target/web/public/test" )  