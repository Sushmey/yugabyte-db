package com.yugabyte.yw.controllers;

import com.google.inject.Inject;
import com.yugabyte.yw.commissioner.Commissioner;
import com.yugabyte.yw.common.PlatformServiceException;
import com.yugabyte.yw.common.Util;
import com.yugabyte.yw.forms.PlatformResults;
import com.yugabyte.yw.forms.PlatformResults.YBPTask;
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
import io.swagger.annotations.ApiOperation;
import java.util.Collections;
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

  @ApiOperation(value = "Remove Addon Service for a cluster", notes = "Remove Addon Service for a cluster")
  public Result removeAddOnService(UUID customerUUID, UUID universeUUID, String addOnName) {
    Universe universe = cloudUtil.checkCloudAndValidateUniverse(customerUUID, universeUUID);

    UniverseDefinitionTaskParams details = universe.getUniverseDetails();
    boolean nodeExists = details.nodeDetailsSet.stream().anyMatch(node -> {
      if (node.isAddonServer && node.nodeName.equals(addOnName)) {
        node.isAddonServer = false;
        return true;
      }
      return false;
    });

    if (!nodeExists) {
      throw new PlatformServiceException(BAD_REQUEST, "Invalid AddOn name:" + addOnName);
    }

    for (NodeDetails node : details.nodeDetailsSet) {
      if (node.isAddonServer && node.nodeName.equals(addOnName)) {
        node.state = NodeState.ToBeRemoved;
      }
    }
    universe.setUniverseDetails(details);
    universe.save();

    UniverseDefinitionTaskParams params = new UniverseDefinitionTaskParams();
    params.nodeDetailsSet = details.nodeDetailsSet;
    params.firstTry = true;
    params.universeUUID = universeUUID;
    params.clusters = details.clusters;


    TaskType taskType = TaskType.RemoveAddOn;
    UUID taskUUID = commissioner.submit(taskType, params);
    Customer customer = Customer.getOrBadRequest(customerUUID);

    CustomerTask.create(customer, universeUUID, taskUUID, TargetType.Universe,
      CustomerTask.TaskType.RemoveAddOn, universe.name);

    return new YBPTask(taskUUID, universeUUID).asResult();
  }

  @ApiOperation(value = "Create AddOn Service for a cluster", notes = "Create AddOn Service for a cluster")
  public Result createAddOnService(UUID customerUUID, UUID universeUUID) {
    Universe universe = cloudUtil.checkCloudAndValidateUniverse(customerUUID, universeUUID);

    // we need a way to get parameters from the request
    // purpose
    // instance type

    NodeDetails nodeDetails = generateNodeDetails(universe, "cdc");

    UniverseDefinitionTaskParams details = universe.getUniverseDetails();
    details.nodeDetailsSet.add(nodeDetails);
    universe.setUniverseDetails(details);
    universe.save();

    UniverseDefinitionTaskParams params = new UniverseDefinitionTaskParams();
    params.nodeDetailsSet = details.nodeDetailsSet;
    params.firstTry = true;
    params.universeUUID = universeUUID;
    params.clusters = details.clusters;

    TaskType taskType = TaskType.CreateAddOn;
    UUID taskUUID = commissioner.submit(taskType, params);
    Customer customer = Customer.getOrBadRequest(customerUUID);

    CustomerTask.create(customer, universeUUID, taskUUID, TargetType.Universe,
      CustomerTask.TaskType.CreateAddOn, universe.name);

    return new YBPTask(taskUUID, universeUUID).asResult();
  }

  private NodeDetails generateNodeDetails(Universe universe, String purpose){
    Cluster cluster = universe.getUniverseDetails().clusters.get(0);

    // create the node details for brand new nodes
    // look at createNodeDetailsWithPlacementIndex()
    NodeDetails nodeDetails = new NodeDetails();
    nodeDetails.placementUuid = cluster.uuid;
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
    nodeDetails.nodeUuid = Util.generateNodeUUID(universe.universeUUID, nodeDetails.nodeName);

    nodeDetails.state = NodeState.ToBeAdded;
    nodeDetails.disksAreMountedByUUID = true;

    return nodeDetails;
  }

  public String generateNodeNameForPurpose(Universe universe, String purpose, int nodeIdx) {
    return universe.getUniverseDetails().nodePrefix + "-" + purpose + "-" + "n" + nodeIdx;
  }

}
