package com.yugabyte.yw.controllers;

import com.google.inject.Inject;
import com.yugabyte.yw.common.cdc.CdcStreamManager;
import com.yugabyte.yw.common.cdc.model.CdcStream;
import com.yugabyte.yw.common.cdc.model.CdcStreamCreateResponse;
import com.yugabyte.yw.common.cdc.model.CdcStreamDeleteResponse;
import com.yugabyte.yw.forms.PlatformResults;
import com.yugabyte.yw.models.Universe;
import io.swagger.annotations.Api;
import io.swagger.annotations.ApiOperation;
import java.util.List;
import java.util.UUID;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import play.mvc.Result;

@Api
public class UniverseCdcStreamController extends AuthenticatedController {

  private static final Logger LOG = LoggerFactory.getLogger(UniverseCdcStreamController.class);

  private final CdcStreamManager cdcStreamManager;
  private final CloudUtil cloudUtil;

  @Inject
  public UniverseCdcStreamController(CloudUtil cloudUtil,
    CdcStreamManager cdcStreamManager) {
    super();
    this.cloudUtil = cloudUtil;
    this.cdcStreamManager = cdcStreamManager;
  }


  @ApiOperation(value = "List CDC Streams for a cluster", notes = "List CDC Streams for a cluster")
  public Result listCdcStreams(UUID customerUUID, UUID universeUUID) throws Exception {
    Universe universe = cloudUtil.checkCloudAndValidateUniverse(customerUUID, universeUUID);

    List<CdcStream> response = cdcStreamManager.getAllCdcStreams(universe);
    return PlatformResults.withData(response);
  }

  @ApiOperation(
    value = "Create CDC Stream for a cluster",
    notes = "Create CDC Stream for a cluster")
  public Result createCdcStream(UUID customerUUID, UUID universeUUID) throws Exception {
    Universe universe = cloudUtil.checkCloudAndValidateUniverse(customerUUID, universeUUID);

    CdcStreamCreateResponse response = cdcStreamManager.createCdcStream(universe, "yugabyte");
    return PlatformResults.withData(response);
  }

  @ApiOperation(
    value = "Delete a CDC stream for a cluster",
    notes = "Delete a CDC Stream for a cluster")
  public Result deleteCdcStream(UUID customerUUID, UUID universeUUID, String streamId)
    throws Exception {
    Universe universe = cloudUtil.checkCloudAndValidateUniverse(customerUUID, universeUUID);

    CdcStreamDeleteResponse response = cdcStreamManager.deleteCdcStream(universe, streamId);
    return PlatformResults.withData(response);
  }
}
