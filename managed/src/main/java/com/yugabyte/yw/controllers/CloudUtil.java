package com.yugabyte.yw.controllers;

import static play.mvc.Http.Status.METHOD_NOT_ALLOWED;

import com.google.inject.Inject;
import com.yugabyte.yw.common.PlatformServiceException;
import com.yugabyte.yw.common.config.RuntimeConfigFactory;
import com.yugabyte.yw.models.Customer;
import com.yugabyte.yw.models.Universe;
import java.util.UUID;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class CloudUtil {

  private static final Logger LOG = LoggerFactory.getLogger(CloudUtil.class);
  private final RuntimeConfigFactory runtimeConfigFactory;


  @Inject
  public CloudUtil(RuntimeConfigFactory runtimeConfigFactory) {

    this.runtimeConfigFactory = runtimeConfigFactory;
  }


  public Universe checkCloudAndValidateUniverse(UUID customerUUID, UUID universeUUID) {
    LOG.info("Checking config for customer='{}', universe='{}'", customerUUID, universeUUID);
    if (!runtimeConfigFactory.globalRuntimeConf().getBoolean("yb.cloud.enabled")) {
      throw new PlatformServiceException(
        METHOD_NOT_ALLOWED, "CDC Stream management is not available.");
    }

    Customer customer = Customer.getOrBadRequest(customerUUID);
    return Universe.getValidUniverseOrBadRequest(universeUUID, customer);
  }
}
