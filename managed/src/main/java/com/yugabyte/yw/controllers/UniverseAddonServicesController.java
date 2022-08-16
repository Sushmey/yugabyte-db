package com.yugabyte.yw.controllers;

import com.google.inject.Inject;
import com.yugabyte.yw.commissioner.Commissioner;
import com.yugabyte.yw.forms.AbstractTaskParams;
import com.yugabyte.yw.forms.PlatformResults;
import com.yugabyte.yw.forms.PlatformResults.YBPTask;
import com.yugabyte.yw.forms.UniverseAddOnTaskParameters;
import com.yugabyte.yw.forms.UniverseDefinitionTaskParams;
import com.yugabyte.yw.forms.UniverseDefinitionTaskParams.Cluster;
import com.yugabyte.yw.models.Customer;
import com.yugabyte.yw.models.CustomerTask;
import com.yugabyte.yw.models.CustomerTask.TargetType;
import com.yugabyte.yw.models.Universe;
import com.yugabyte.yw.models.helpers.CloudSpecificInfo;
import com.yugabyte.yw.models.helpers.NodeDetails;
import com.yugabyte.yw.models.helpers.NodeDetails.NodeState;
import com.yugabyte.yw.models.helpers.PlacementInfo.PlacementAZ;
import com.yugabyte.yw.models.helpers.PlacementInfo.PlacementCloud;
import com.yugabyte.yw.models.helpers.PlacementInfo.PlacementRegion;
import com.yugabyte.yw.models.helpers.TaskType;
import io.swagger.annotations.Api;
import io.swagger.annotations.ApiModelProperty;
import io.swagger.annotations.ApiOperation;
import java.util.Collections;
import java.util.Set;
import java.util.UUID;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import play.mvc.Result;

@Api
public class UniverseAddonServicesController extends AuthenticatedController {

  private static final Logger LOG = LoggerFactory.getLogger(UniverseAddonServicesController.class);
  private final CloudUtil cloudUtil;
  private final Commissioner commissioner;

  @Inject
  public UniverseAddonServicesController(CloudUtil cloudUtil, Commissioner commissioner) {
    super();
    this.cloudUtil = cloudUtil;
    this.commissioner = commissioner;
  }

  @ApiOperation(value = "List Addon Services for a cluster", notes = "List Addon Services for a cluster")
  public Result listAddonServices(UUID customerUUID, UUID universeUUID) throws Exception {
    Universe universe = cloudUtil.checkCloudAndValidateUniverse(customerUUID, universeUUID);

    //List<DependentService> response = cdcStreamManager.getAllDependentServices(universe);
    String response = "";
    return PlatformResults.withData(response);
  }

  @ApiOperation(value = "Create AddOn Service for a cluster", notes = "Create AddOn Service for a cluster")
  public Result createAddOnService(UUID customerUUID, UUID universeUUID) {
    Universe universe = cloudUtil.checkCloudAndValidateUniverse(customerUUID, universeUUID);

    // we need a way to get parameters from the request

    // validations
    // * VM name?
    // * node exporter?




    NodeDetails nodeDetails = generateNodeDetails(universe, "cdc");

    UniverseDefinitionTaskParams details = universe.getUniverseDetails();
    details.nodeDetailsSet.add(nodeDetails);
    universe.setUniverseDetails(details);
    universe.save();

    UniverseAddOnTaskParameters params = new UniverseAddOnTaskParameters();
    params.nodeDetailsSet = Collections.singleton(nodeDetails);
    params.firstTry = true;
    params.universeUUID = universeUUID;

    TaskType taskType = TaskType.CreateAddOn;
    UUID taskUUID = commissioner.submit(taskType, params);
    Customer customer = Customer.getOrBadRequest(customerUUID);

    CustomerTask.create(customer, universeUUID, taskUUID, TargetType.Universe,
      CustomerTask.TaskType.UpgradeGflags, universe.name);

    return new YBPTask(taskUUID, universeUUID).asResult();
  }

  private NodeDetails generateNodeDetails(Universe universe, String purpose){
    Cluster cluster = universe.getUniverseDetails().clusters.get(0);

    // create the node details for brand new nodes
    // look at createNodeDetailsWithPlacementIndex()
    NodeDetails nodeDetails = new NodeDetails();
    nodeDetails.placementUuid = cluster.uuid;
    nodeDetails.machineImage = "";
    nodeDetails.ybPrebuiltAmi = false;

    int maxNodeIndex = -1;
    for (NodeDetails existingNode : universe.getNodes()) {
      if (maxNodeIndex < existingNode.nodeIdx) {
        maxNodeIndex = existingNode.nodeIdx;
      }
    }

    PlacementCloud placementCloud = cluster.placementInfo.cloudList.get(0);
    PlacementRegion placementRegion = placementCloud.regionList.get(0);

    // Set the AZ and the subnet.
    PlacementAZ placementAZ = placementRegion.azList.get(0);
    nodeDetails.azUuid = placementAZ.uuid;

    nodeDetails.cloudInfo = new CloudSpecificInfo();
    nodeDetails.cloudInfo.cloud = placementCloud.code;
    nodeDetails.cloudInfo.az = placementAZ.name;
    nodeDetails.cloudInfo.subnet_id = placementAZ.subnet;
    nodeDetails.cloudInfo.secondary_subnet_id = placementAZ.secondarySubnet;
    nodeDetails.cloudInfo.instance_type = cluster.userIntent.instanceType;
    nodeDetails.cloudInfo.assignPublicIP = cluster.userIntent.assignPublicIP;
    nodeDetails.cloudInfo.useTimeSync = cluster.userIntent.useTimeSync;
    nodeDetails.cloudInfo.region = placementRegion.code;

    nodeDetails.isTserver = false;
    nodeDetails.isMaster = false;
    nodeDetails.isAddonServer = true;

    nodeDetails.nodeIdx = (maxNodeIndex + 1);
    nodeDetails.nodeName = generateNodeNameForPurpose(universe, purpose, nodeDetails.nodeIdx);

    nodeDetails.state = NodeState.ToBeAdded;
    nodeDetails.disksAreMountedByUUID = true;

    return nodeDetails;
  }

  public String generateNodeNameForPurpose(Universe universe, String purpose, int nodeIdx) {
    return universe.getUniverseDetails().nodePrefix + "-" + purpose + "-" + "n" + nodeIdx;
  }

}
