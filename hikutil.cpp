#include "HCNetSDK.h"
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::chrono;

void SDK_Version() {

  unsigned int uiVersion = NET_DVR_GetSDKBuildVersion();

  char strTemp[1024] = {0};
  sprintf(strTemp, "HCNetSDK V%d.%d.%d.%d\n", (0xff000000 & uiVersion) >> 24,
          (0x00ff0000 & uiVersion) >> 16, (0x0000ff00 & uiVersion) >> 8,
          (0x000000ff & uiVersion));
  printf(strTemp);
}

int saveFile(int userId, char *srcfile, char *destfile) {
  printf("Save %s\n", srcfile);
  int bRes = 1;

  int hPlayback = 0;
  if ((hPlayback = NET_DVR_GetFileByName(userId, srcfile, destfile)) < 0) {
    printf("GetFileByName failed. error[%d]\n", NET_DVR_GetLastError());
    bRes = -1;
    return bRes;
  }

  if (!NET_DVR_PlayBackControl(hPlayback, NET_DVR_PLAYSTART, 0, NULL)) {
    printf("play back control failed [%d]\n", NET_DVR_GetLastError());
    bRes = -1;
    return bRes;
  }

  int pos = 0;
  int lastpos = 0;
  for (pos = 0; pos < 100 && pos >= 0;
       pos = NET_DVR_GetDownloadPos(hPlayback)) {
    if (lastpos != pos) {
      printf("%d%\r", pos);
      fflush(stdout);
      lastpos = pos;
    }
  }
  printf("\n");

  if (!NET_DVR_StopGetFile(hPlayback)) {
    printf("failed to stop get file [%d]\n", NET_DVR_GetLastError());
    bRes = -1;
    return bRes;
  }

  printf("%s\n", destfile);

  if (pos < 0 || pos > 100) {
    printf("download err [%d]\n", NET_DVR_GetLastError());
    bRes = -1;
    return bRes;
  } else {
    return 0;
  }
}

int CaptureImage(long lUserID, NET_DVR_DEVICEINFO_V40 struDeviceInfoV40,
                 char *imagePath) {
  printf("Capture image.\n");
  NET_DVR_JPEGPARA strPicPara = {0};
  strPicPara.wPicQuality = 0;
  strPicPara.wPicSize = 0xff;
  int iRet;
  auto start = high_resolution_clock::now();
  iRet = NET_DVR_CaptureJPEGPicture(lUserID,
                                    struDeviceInfoV40.struDeviceV30.byStartChan,
                                    &strPicPara, imagePath);
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  printf("%d\n", duration.count());
  if (!iRet) {
    printf("NET_DVR_CaptureJPEGPicture error, %d\n", NET_DVR_GetLastError());
    return -1;
  }
  return 0;
}

int ListVideo(long lUserID, char *downloadPath) {
  printf("List video.\n");

  auto now = system_clock::now();
  time_t tt = system_clock::to_time_t(now);
  tm local_tm = *localtime(&tt);

  printf("%d\n", local_tm.tm_year);

  NET_DVR_FILECOND_V50 m_struFileCondV50;
  memset(&m_struFileCondV50, 0, sizeof(NET_DVR_FILECOND_V50));
  m_struFileCondV50.struStreamID.dwChannel = 1;
  // m_struFileCondV50.dwFileType = 0xff;
  m_struFileCondV50.dwFileType = 255;

  m_struFileCondV50.struStartTime.wYear = local_tm.tm_year + 1900;
  m_struFileCondV50.struStartTime.byMonth = local_tm.tm_mon + 1;
  m_struFileCondV50.struStartTime.byDay = 1;
  m_struFileCondV50.struStartTime.byHour = 0;
  m_struFileCondV50.struStartTime.byMinute = 0;
  m_struFileCondV50.struStartTime.bySecond = 0;

  m_struFileCondV50.struStopTime.wYear = local_tm.tm_year + 1900;
  m_struFileCondV50.struStopTime.byMonth = local_tm.tm_mon + 1;
  m_struFileCondV50.struStopTime.byDay = local_tm.tm_mday;
  m_struFileCondV50.struStopTime.byHour = 23;
  m_struFileCondV50.struStopTime.byMinute = 59;
  m_struFileCondV50.struStopTime.bySecond = 59;

  int lFindHandle = NET_DVR_FindFile_V50(lUserID, &m_struFileCondV50);

  if (lFindHandle < 0) {
    printf("find file fail,last error %d\n", NET_DVR_GetLastError());
    return 1;
  }
  NET_DVR_FINDDATA_V50 struFileData;

  int count = 0;
  while (true) {
    int result = NET_DVR_FindNextFile_V50(lFindHandle, &struFileData);
    if (result == NET_DVR_ISFINDING) {
      continue;
    } else if (result == NET_DVR_FILE_SUCCESS) {
      count++;

      printf(
          "%d:%s, [%db] %d-%d-%d %d:%d:%d -- %d-%d-%d %d:%d:%d.\n", count,
          struFileData.sFileName, struFileData.dwFileSize,
          struFileData.struStartTime.wYear, struFileData.struStartTime.byMonth,
          struFileData.struStartTime.byDay, struFileData.struStartTime.byHour,
          struFileData.struStartTime.byMinute,
          struFileData.struStartTime.bySecond, struFileData.struStopTime.wYear,
          struFileData.struStopTime.byMonth, struFileData.struStopTime.byDay,
          struFileData.struStopTime.byHour, struFileData.struStopTime.byMinute,
          struFileData.struStopTime.bySecond);
      if (strlen(downloadPath) > 0) {
        char todir[1000] = "";
        char checktodir[1000] = "";
        sprintf(todir, "%s/%d-%02d-%02d", downloadPath,
                struFileData.struStartTime.wYear,
                struFileData.struStartTime.byMonth,
                struFileData.struStartTime.byDay);
        mkdir(todir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        sprintf(checktodir, "%s/%02d:%02d:%02d.mp4", todir,
                struFileData.struStartTime.byHour,
                struFileData.struStartTime.byMinute,
                struFileData.struStartTime.bySecond);
        sprintf(todir, "%s/%02d:%02d:%02d.mpeg", todir,
                struFileData.struStartTime.byHour,
                struFileData.struStartTime.byMinute,
                struFileData.struStartTime.bySecond);

        if (access(checktodir, F_OK) == -1) {
          saveFile(lUserID, struFileData.sFileName, todir);
        }
      }
      // break;
      continue;
    } else if (result == NET_DVR_FILE_NOFIND || result == NET_DVR_NOMOREFILE) {
      printf("No more files!\n");
      break;
    } else {
      printf("find file fail for illegal get file state\n");
      break;
    }

    NET_DVR_FindClose_V30(lFindHandle);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  char optString[100] = "lu:h:p:o:d:D:r?";
  char download[100] = "";
  char logPath[10] = "./sdkLog";
  char ip[100] = "";
  char username[100] = "";
  char password[100] = "";
  char imagePath[100] = "./out.jpg";
  bool listVideo = false;
  bool reboot = false;
  char downloadPath[1000] = "";

  int opt = getopt(argc, argv, optString);
  while (opt != -1) {
    switch (opt) {
    case 'u':
      strncpy(username, optarg, 100);
      break;
    case 'h':
      strncpy(ip, optarg, 100);
      break;
    case 'p':
      strncpy(password, optarg, 100);
      break;
    case 'o':
      strncpy(imagePath, optarg, 100);
      break;
    case 'd':
      strncpy(download, optarg, 100);
      break;
    case 'D':
      strncpy(downloadPath, optarg, 100);
      break;
    case 'l':
      listVideo = true;
      break;
    case 'r':
      reboot = true;
      break;
    case '?':
      printf("Utils for HIKVISION CAMS.");
      break;

    default:
      printf("Default.");
      break;
    }

    opt = getopt(argc, argv, optString);
  }

  if (strlen(username) > 0 && strlen(password) > 0 && strlen(ip) > 0) {

    printf("username = %s camera = %s\n", username, ip);

    NET_DVR_Init();
    SDK_Version();
    NET_DVR_SetLogToFile(1, logPath);
    // char cUserChoose = 'r';

    // Login device
    NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
    struLoginInfo.bUseAsynLogin = false;

    struLoginInfo.wPort = 8000;
    memcpy(struLoginInfo.sDeviceAddress, ip, NET_DVR_DEV_ADDRESS_MAX_LEN);
    memcpy(struLoginInfo.sUserName, username, NAME_LEN);
    memcpy(struLoginInfo.sPassword, password, NAME_LEN);

    int lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);

    if (lUserID < 0) {
      printf("Login error, %d\n", NET_DVR_GetLastError());

      NET_DVR_Cleanup();
      return -1;
    }

    //
    if (listVideo || strlen(downloadPath) > 0) {
      ListVideo(lUserID, downloadPath);
    } else if (strlen(download) > 0) {
      saveFile(lUserID, download, imagePath);
    } else if (reboot) {
      printf("Reboot device...");
      NET_DVR_RebootDVR(lUserID);
    } else {
      CaptureImage(lUserID, struDeviceInfoV40, imagePath);
    }
    // logout
    NET_DVR_Logout_V30(lUserID);
    NET_DVR_Cleanup();

    return 0;
  } else {
    printf("Set username (-u),password (-p),host (-h).\n");
    printf("\t-u username.\n");
    printf("\t-p password.\n");
    printf("\t-h cam ip.\n");
    printf("\t-l list captured video for current month.\n");
    printf("\t-D download captured video to specified path.\n");
    printf("\t-r reboot.\n");
    printf("\t-o capture snapshot to specified path.\n");

    return 0;
  }
}
