package com.yugabyte.yw.commissioner.tasks;

import com.yugabyte.yw.commissioner.AbstractTaskBase;
import com.yugabyte.yw.commissioner.BaseTaskDependencies;
import com.yugabyte.yw.commissioner.ITask.Abortable;
import com.yugabyte.yw.commissioner.ITask.Retryable;
import com.yugabyte.yw.forms.UniverseAddOnTaskParameters;
import lombok.extern.slf4j.Slf4j;

@Slf4j
@Abortable
@Retryable
public class CreateAddOn extends AbstractTaskBase {

  public CreateAddOn(BaseTaskDependencies baseTaskDependencies) {
    super(baseTaskDependencies);
  }

  @Override
  protected UniverseAddOnTaskParameters taskParams() {
    return (UniverseAddOnTaskParameters) taskParams;
  }

  @Override
  public void run() {
    log.info("Started {} task for uuid={}", getName(), taskParams().universeUUID);

    log.info("We would need to provision the following nodes: {}", taskParams().nodeDetailsSet);
  }


}
