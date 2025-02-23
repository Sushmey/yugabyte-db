---
title: Client drivers for YCQL API
headerTitle: Client drivers for YCQL
linkTitle: Client drivers for YCQL
description: Lists the client drivers that you can use to build and access YCQL applications.
menu:
  preview:
    identifier: ycql-client-libraries
    parent: drivers
    weight: 2942
aliases:
type: docs
---

The following client drivers are supported for use with the [Yugabyte Cloud Query Language (YCQL) API](../../../api/ycql/), a SQL-based, semi-relational API, with roots in the Apache Cassandra Query Language (CQL).

For tutorials on building a sample application with the following client drivers, click the relevant link included below for each driver.

{{< note title="Use YugabyteDB client drivers" >}}
You should always use the YugabyteDB YCQL client drivers. Using generic Cassandra drivers can lead to errors and performance issues.
{{< /note >}}

## C/C++

### Yugabyte C/C++ Driver for YCQL

The [Yugabyte C++ Driver for YCQL](https://github.com/yugabyte/cassandra-cpp-driver) is based on the [DataStax C++ Driver for Apache Cassandra](https://github.com/datastax/cpp-driver).

For details, see the [README](https://github.com/yugabyte/cassandra-cpp-driver) in our GitHub repository.

For a tutorial on building a sample C++ application with this driver, see [Build a C++ application](../../../develop/build-apps/cpp/ycql/).

## C\#

### Yugabyte C# Driver for YCQL

The [Yugabyte C# Driver for YCQL](https://github.com/yugabyte/cassandra-csharp-driver) is based on a fork of the [DataStax C# Driver for Apache Cassandra](https://github.com/datastax/csharp-driver).

For details, see the [README](https://github.com/yugabyte/cassandra-csharp-driver) in our GitHub repository.

For a tutorial on building a sample C# application with this driver, see [Build a C# application](../../../develop/build-apps/csharp/ycql/).

## Go

### Yugabyte Go Driver for YCQL

The [Yugabyte Go Driver for YCQL](https://github.com/yugabyte/gocql) is based on a fork of [GoCQL](http://gocql.github.io/).

For details, see the [README](https://github.com/yugabyte/gocql/blob/master/README.md) in our GitHub repository.

For a tutorial on building a sample Go application with this driver, see [Build a Go application](../../../develop/build-apps/go/ycql/).

## Java

### Yugabyte Java Driver for YCQL 4.6

The [Yugabyte Java Driver for YCQL](https://github.com/yugabyte/cassandra-java-driver/tree/4.6.0-yb-x/manual/core), version `4.6.0-yb-x`, is based on the [DataStax Java Driver for Apache Cassandra (v4.6)](https://github.com/datastax/java-driver) and requires the Maven dependency shown below.

For details, see the [v4.6 README](https://github.com/yugabyte/cassandra-java-driver/blob/4.6.0-yb-x/README.md) in our GitHub repository.

For a tutorial on building a sample Java application with this driver, see [Build a Java application](../../../develop/build-apps/java/ycql/).

To build Java applications with this driver, you must add the following Maven dependency to your application:

```xml
<dependency>
  <groupId>com.yugabyte</groupId>
  <artifactId>java-driver-core</artifactId>
  <version>4.6.0-yb-11</version>
</dependency>
```

For details, see the [Maven repository contents](https://mvnrepository.com/artifact/com.yugabyte/java-driver-core/4.6.0-yb-11).

### Yugabyte Java Driver for YCQL 3.10

The [Yugabyte Java Driver for YCQL](https://github.com/yugabyte/cassandra-java-driver), version `3.10.0-yb-x`, is based on the [DataStax Java Driver for Apache Cassandra v.3.10](https://github.com/datastax/java-driver) and requires the Maven dependency shown below.

For details, see the [v3.10 README](https://github.com/yugabyte/cassandra-java-driver/blob/3.10.0-yb-x/README.md) in our GitHub repository.

To build Java applications with this driver, you must add the following Maven dependency to your application:

```xml
<dependency>
  <groupId>com.yugabyte</groupId>
  <artifactId>cassandra-driver-core</artifactId>
  <version>3.10.3-yb-2</version>
</dependency>
```

For details, see the [Maven repository contents](https://mvnrepository.com/artifact/com.yugabyte/cassandra-driver-core/3.10.3-yb-2).

## Node.js

### Yugabyte Node.js driver for YCQL

The [YugabyteDB Node.js driver for YCQL](https://github.com/yugabyte/cassandra-nodejs-driver) is based on a fork of the [DataStax Node.js Driver for Apache Cassandra](https://github.com/datastax/nodejs-driver).

For details, see the [README](https://github.com/datastax/cpp-driver/blob/master/README.md) in our GitHub repository.

For a tutorial on building a sample Node.js application with this driver, see [Build a Node.js application](../../../develop/build-apps/nodejs/ycql/).

## Python

### Yugabyte Python Driver for YCQL

The [Yugabyte Python Driver for YCQL](https://github.com/yugabyte/cassandra-python-driver) is based on a fork of the [DataStax Python Driver for Apache Cassandra](https://github.com/datastax/python-driver).

For details, see the [README](https://github.com/yugabyte/cassandra-python-driver) in our GitHub repository.

For a tutorial on building a sample Python application with this driver, see [Build a Python application](../../../develop/build-apps/python/ycql/).

## Ruby

### Yugabyte Ruby Driver for YCQL

The [Yugabyte Ruby Driver for YCQL](https://github.com/yugabyte/cassandra-ruby-driver) is based on a fork of the [DataStax Ruby Driver for Apache Cassandra](https://github.com/datastax/ruby-driver).

For details, see the [README](https://github.com/yugabyte/cassandra-ruby-driver/blob/v3.2.3.x-yb/README.md) in our GitHub repository.

For a tutorial on building a sample Ruby application with this driver, see [Build a Ruby application](../../../develop/build-apps/ruby/ycql/).

## Scala

### Yugabyte Java Driver for YCQL

The [Yugabyte Java Driver for YCQL](https://github.com/yugabyte/cassandra-java-driver) is based on a fork of the [DataStax Java Driver for Apache Cassandra](https://github.com/datastax/java-driver) and can be used to build Scala applications when you add the [`sbt` (Scala build tool)](https://www.scala-sbt.org/1.x/docs/index.html) dependency shown below to your application.

For details, see the [README](https://github.com/yugabyte/cassandra-java-driver/blob/3.8.0-yb-x/README.md) in our GitHub repository.

To build a Scala application with the Yugabyte Java Driver for YCQL, you must add the following `sbt` dependency to your application:

```sh
libraryDependencies += "com.yugabyte" % "cassandra-driver-core" % "3.8.0-yb-5"
```

For a tutorial on building a sample Scala application with this driver, see [Build a Scala application](../../../develop/build-apps/scala/ycql/).
