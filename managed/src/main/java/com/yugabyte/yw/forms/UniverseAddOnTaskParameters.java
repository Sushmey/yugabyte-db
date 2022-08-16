package com.yugabyte.yw.forms;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.yugabyte.yw.models.helpers.NodeDetails;
import io.swagger.annotations.ApiModelProperty;
import java.util.Set;
import java.util.UUID;

@JsonIgnoreProperties(ignoreUnknown = true)
public class UniverseAddOnTaskParameters extends AbstractTaskParams {
  // The universe against which this operation is being executed.
  @ApiModelProperty(value = "Associated universe UUID")
  public UUID universeUUID;

  // The set of nodes that are part of this universe. Should contain nodes in both primary and
  // readOnly clusters.
  @ApiModelProperty(value = "Node details")
  public Set<NodeDetails> nodeDetailsSet = null;

  // Whether this task has been tried before or not. Awkward naming because we cannot use
  // `isRetry` due to play reading the "is" prefix differently.
  @ApiModelProperty(value = "Whether this task has been tried before")
  public boolean firstTry = true;

  // Previous task UUID for a retry.
  @ApiModelProperty(value = "Previous task UUID only if this task is a retry")
  public UUID previousTaskUUID;
}
