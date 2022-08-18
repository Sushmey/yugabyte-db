package com.yugabyte.yw.commissioner.tasks;

import com.google.inject.Inject;
import com.yugabyte.yw.commissioner.BaseTaskDependencies;
import com.yugabyte.yw.commissioner.ITask.Abortable;
import com.yugabyte.yw.commissioner.ITask.Retryable;
import com.yugabyte.yw.commissioner.UserTaskDetails.SubTaskGroupType;
import com.yugabyte.yw.models.Universe;
import com.yugabyte.yw.models.helpers.NodeDetails;
import com.yugabyte.yw.models.helpers.NodeDetails.NodeState;
import java.util.Set;
import java.util.UUID;
import java.util.stream.Collectors;
import lombok.extern.slf4j.Slf4j;

@Slf4j
@Abortable
@Retryable
public class CreateAddOn extends UniverseDefinitionTaskBase {

  @Inject
  public CreateAddOn(BaseTaskDependencies baseTaskDependencies) {
    super(baseTaskDependencies);
  }

  @Override
  public void run() {
    String errorString = null;
    UUID universeUUID = taskParams().universeUUID;
    log.info("Started {} task for uuid={}", getName(), universeUUID);

    try {
      Set<NodeDetails> nodesToProvision = taskParams().nodeDetailsSet.stream()
        .filter(n -> n.state == NodeState.ToBeAdded && n.isAddonServer)
        .collect(Collectors.toSet());

      log.info("We would need to provision the following nodes: {}",
        nodesToProvision);

      Universe universe = lockUniverseForUpdate(-1 /* always lock */, u -> {
        // do validations here
      });

      createAddOn(universe, nodesToProvision);

      // <<<< Maybe not needed?
      // Marks the update of this universe as a success only if all the tasks before it succeeded.
      createMarkUniverseUpdateSuccessTasks()
        .setSubTaskGroupType(SubTaskGroupType.ConfigureUniverse);

      // Run all the tasks.
      getRunnableTask().runSubTasks();
    } catch (Throwable t) {
      log.error("Error executing task {} with error='{}'.", getName(), t.getMessage(), t);
      errorString = t.getMessage();
      throw t;
    } finally {
      Universe universe = unlockUniverseForUpdate(errorString);

      log.info("Finished {} task.", getName());
    }
  }

  protected void createAddOn(Universe universe, Set<NodeDetails> nodesToProvision) {
    boolean ignoreUseCustomImageConfig = false;

    createProvisionNodeTasks(
      universe,
      nodesToProvision,
      true /* isShell */,
      false /* ignore node status check */,
      ignoreUseCustomImageConfig);

  }


}
