package org.yb.util;

public class SystemUtil {
  public static final boolean IS_LINUX =
    System.getProperty("os.name").equalsIgnoreCase("linux");
}
