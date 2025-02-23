package com.yugabyte.yw.common;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mockito.MockitoAnnotations.initMocks;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.yugabyte.yw.common.services.YBClientService;
import com.yugabyte.yw.forms.BackupTableParams;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import junitparams.JUnitParamsRunner;
import junitparams.Parameters;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;

@RunWith(JUnitParamsRunner.class)
public class BackupUtilTest extends FakeDBApplication {

  private static final Map<String, String> REGION_LOCATIONS =
      new HashMap<String, String>() {
        {
          put("us-west1", "s3://backups.yugabyte.com/test/user/reg1");
          put("us-west2", "s3://backups.yugabyte.com/test/user/reg2");
          put("us-east1", "s3://backups.yugabyte.com/test/user/reg3");
        }
      };

  @InjectMocks BackupUtil backupUtil;

  @Mock YBClientService ybService;

  @Before
  public void setup() {
    initMocks(this);
  }

  @Test(expected = Test.None.class)
  @Parameters({"0 */2 * * *", "0 */3 * * *", "0 */1 * * *", "5 */1 * * *", "1 * * * 2"})
  public void testBackupCronExpressionValid(String cronExpression) {
    BackupUtil.validateBackupCronExpression(cronExpression);
  }

  @Test
  @Parameters({"*/10 * * * *", "*/50 * * * *"})
  public void testBackupCronExpressionInvalid(String cronExpression) {
    Exception exception =
        assertThrows(
            PlatformServiceException.class,
            () -> BackupUtil.validateBackupCronExpression(cronExpression));
    assertEquals(
        "Duration between the cron schedules cannot be less than 1 hour", exception.getMessage());
  }

  @SuppressWarnings("unused")
  private Object[] paramsToValidateFrequency() {
    return new Object[] {4800000L, 3600000L};
  }

  @Test(expected = Test.None.class)
  @Parameters(method = "paramsToValidateFrequency")
  public void testBackupFrequencyValid(Long frequency) {
    BackupUtil.validateBackupFrequency(frequency);
  }

  @SuppressWarnings("unused")
  private Object[] paramsToInvalidateFrequency() {
    return new Object[] {1200000L, 2400000L};
  }

  @Test
  @Parameters(method = "paramsToInvalidateFrequency")
  public void testBackupFrequencyInvalid(Long frequency) {
    Exception exception =
        assertThrows(
            PlatformServiceException.class, () -> BackupUtil.validateBackupFrequency(frequency));
    assertEquals("Minimum schedule duration is 1 hour", exception.getMessage());
  }

  @SuppressWarnings("unused")
  private Object[] getBackupSuccessData() {
    String backupSuccessWithNoRegions = "backup/backup_success_with_no_regions.json";

    String backupSuccessWithRegions = "backup/backup_success_with_regions.json";

    return new Object[] {
      new Object[] {backupSuccessWithNoRegions, 0},
      new Object[] {backupSuccessWithRegions, 3}
    };
  }

  @Test
  @Parameters(method = "getBackupSuccessData")
  public void testExtractbackupLocations(String dataFile, int expectedCount) throws IOException {
    ObjectMapper mapper = new ObjectMapper();
    JsonNode locations = mapper.readTree(TestUtils.readResource(dataFile));
    List<BackupUtil.RegionLocations> actualLocations =
        BackupUtil.extractPerRegionLocationsFromBackupScriptResponse(locations);
    if (expectedCount == 0) {
      assertTrue(actualLocations.size() == 0);
    } else {
      Map<String, String> regionLocations = new HashMap<>();
      actualLocations.forEach(aL -> regionLocations.put(aL.REGION, aL.LOCATION));
      assertEquals(regionLocations.size(), REGION_LOCATIONS.size());
      assertTrue(regionLocations.equals(REGION_LOCATIONS));
    }
  }

  @Test
  @Parameters(
      value = {
        "/tmp/nfs/, /tmp/nfs//yugabyte_backup/foo, yugabyte_backup/foo",
        "/tmp/nfs, /tmp/nfs/yugabyte_backup/foo, yugabyte_backup/foo",
        "/tmp/nfs/, /tmp/nfs//foo, foo",
        "s3://backup, s3://backup/foo, foo",
        "s3://backup/, s3://backup//foo, foo"
      })
  public void testGetBackupIdentifierWithoutNfsCheck(
      String configDefaultLocation, String defaultBackupLocation, String expectedIdentifier) {
    String actualIdentifier =
        BackupUtil.getBackupIdentifier(configDefaultLocation, defaultBackupLocation, false);
    assertEquals(expectedIdentifier, actualIdentifier);
  }

  @Test
  @Parameters(
      value = {
        "/tmp/nfs/, /tmp/nfs//yugabyte_backup/foo, foo",
        "/tmp/nfs, /tmp/nfs/yugabyte_backup/foo, foo",
        "/tmp/nfs/, /tmp/nfs//foo, foo",
        "s3://backup, s3://backup/foo, foo",
        "s3://backup/, s3://backup//foo, foo"
      })
  public void testGetBackupIdentifierWithNfsCheck(
      String configDefaultLocation, String defaultBackupLocation, String expectedIdentifier) {
    String actualIdentifier =
        BackupUtil.getBackupIdentifier(configDefaultLocation, defaultBackupLocation, true);
    assertEquals(expectedIdentifier, actualIdentifier);
  }

  @Test
  @Parameters(
      value = {
        "s3://backup/foo, s3://backup, s3://region/, s3://region//foo",
        "s3://backup/foo, s3://backup, s3://region, s3://region/foo",
        "s3://backup//foo, s3://backup/, s3://region, s3://region/foo"
      })
  public void getExactRegionLocation(
      String backupLocation,
      String configDefaultLocation,
      String configRegionLocation,
      String expectedRegionLocation) {
    String actualRegionLocation =
        BackupUtil.getExactRegionLocation(
            backupLocation, configDefaultLocation, configRegionLocation);
    assertEquals(expectedRegionLocation, actualRegionLocation);
  }

  @Test
  @Parameters(value = {"true, true", "true, false", "false, false", "false, true"})
  public void testBackupLocationFormat(boolean emptyTableList, boolean isYbc) {
    BackupTableParams tableParams = new BackupTableParams();
    tableParams.universeUUID = UUID.randomUUID();
    tableParams.backupUuid = UUID.randomUUID();
    tableParams.setKeyspace("foo");
    if (emptyTableList) {
      tableParams.tableUUIDList = null;
    } else {
      tableParams.tableUUIDList = new ArrayList<>();
    }
    String formattedLocation = BackupUtil.formatStorageLocation(tableParams, isYbc);
    if (isYbc) {
      assertTrue(formattedLocation.contains("/ybc_backup"));
      if (emptyTableList) {
        assertTrue(formattedLocation.contains("/keyspace-foo"));
      } else {
        assertTrue(formattedLocation.contains("/multi-table-foo"));
      }
    } else {
      assertTrue(formattedLocation.contains("/backup"));
      if (emptyTableList) {
        assertTrue(formattedLocation.contains("/keyspace-foo"));
      } else {
        assertTrue(formattedLocation.contains("/multi-table-foo"));
      }
    }
  }

  @Test
  @Parameters(
      value = {
        "s3://foo, s3://foo/univ-318eef98-044b-4293-b560-73ef2e1f2df9/ybc_backup-foo/bar, true",
        "s3://foo, s3://foo/univ-318EEf98-044b-4293-b560-73ef2e1f2df9/ybc_backup-foo/bar, true",
        "s3://foo, s3://foo/univ-318eef98-044B-42A3-b560-73ef2e1f2df9/ybc_backup-foo/bar, true",
        "s3://foo, s3://foo/univ-318eef98-044b-4293-b560-73ef2e1f2df9/backup_ybc-foo/bar, false",
        "s3://foo, s3://foo/univ-318eef98-044b-4293-b560-73ef2e1f2df9/backup-foo/bar_ybc, false",
        "s3://foo, s3://foo/univ-318eef98-044b-4293-b560-73ef2e1f2df9/backup-foo/ybc_backup, false",
        "/tmp/nfs, /tmp/nfs/univ-318eef98-044b-4293-b560-73ef2e1f2df9/backup-foo/ybc_backup, false",
        "/tmp/nfs, /tmp/nfs/univ-318eef98-044b-4293-b560-73ef2e1f2df9/ybc_backup-foo/bar, true",
        "/nfs, /nfs/yugabyte_backup/univ-318eef98-044b-4293-b560-73ef2e1f2df9/ybc_backup-foo/bar"
            + ", true"
      })
  public void testIsYbcBackup(String configLocation, String backupLocation, boolean expected) {
    boolean actual = backupUtil.isYbcBackup(configLocation, backupLocation);
    assertEquals(expected, actual);
  }
}
