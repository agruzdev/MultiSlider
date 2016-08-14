name := "MS_Watchdog"

version := "1.0"

lazy val `ms_watchdog` = (project in file(".")).enablePlugins(PlayScala)

scalaVersion := "2.11.2"

scalacOptions += "-feature"

scalacOptions += "-language:postfixOps"

libraryDependencies ++= Seq( jdbc , anorm , cache , ws )

// http://mvnrepository.com/artifact/net.liftweb/lift-json_2.11
libraryDependencies += "net.liftweb" % "lift-json_2.11" % "3.0-M8"

// https://mvnrepository.com/artifact/com.typesafe.akka/akka-remote_2.11
libraryDependencies += "com.typesafe.akka" % "akka-remote_2.11" % "2.3.6"

// https://mvnrepository.com/artifact/io.netty/netty
libraryDependencies += "io.netty" % "netty" % "3.8.0.Final"

// https://mvnrepository.com/artifact/com.google.protobuf/protobuf-java
libraryDependencies += "com.google.protobuf" % "protobuf-java" % "2.5.0"


unmanagedResourceDirectories in Test <+=  baseDirectory ( _ /"target/web/public/test" )  