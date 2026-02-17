# MineRecord EX

MineRecordEX is an application that keeps a record of and tracks in-game character stats during livestreamed gameplay for the popular Minecraft video game. This utility program was developed in Python 3, PostgreSQL and C++. The associated Minecraft Java Edition mod was developed in Java 21. As of right now, this application only has native support for Minecraft Java Edition. However, if this software gets popular later on in the future, I may consider adding support for other video games.

<img width="200" height="200" alt="MREX Icon" src="https://github.com/user-attachments/assets/498bb76c-b020-483f-a7ce-3b37613c1d66" />


## Disclosure Message:
Please note that while a large portion of the codebase for this application was programmed manually by me, Gen AI tools were still used to help apply bug fixes as well as optimize its performance and efficiency. All this boils down to is that Gen AI was used as an assistive tool and not a replacement for development.

## Install Instructions:
1. Make sure you have Minecraft Java Edition installed onto your PC (preferrably build version 1.21.10 as its the one that I used to test the functionality of the application). I also highly recommend using prism launcher as Microsoft's official Minecraft launcher is incompatible with this program.
2. Install  64-bit PostgreSQL build version 17.7. You can find the download links [here](https://www.enterprisedb.com/downloads/postgres-postgresql-downloads)
3. Download MineRecordEX and the associated minecraft mod files from the releases page [here](https://github.com/m24faisal/MineRecord-EX/releases/tag/v1.0)
4. Install the 64-bit Microsoft Visual C++ Redistributable executable file [here](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-supported-redistributable-version)

If anyone wants a visual demonstration on how to download and setup the app, click here

## Usage Instructions
1. Make sure you followed the Installation Instructions noted above before you launch the app for the first
2. Go to File --> "Add Game" and proceed to add your official Minecraft launcher executable file to the main MineRecordEX application table
3. Select the launcher from the MineRecordEX main window's item list, right click and select Start Recording to start recording your game play (if you wish to screen record your gameplay and track your stats) or just run the minecraft instance and enable the mod normally through the Minecraft launch and MineRecordEX should easily detect that the launcher is currently running. The latter option is more meant for users who just want to track player stats and not screen record gameplay. If there are any issues here, please make sure that run as admin is disabled first before running the launcher.
4. After you are done playing a session in Minecraft, go ahead and exit out of the application. Once you have done that, if you are done recording, either right click your minecraft launcher exe in the list and select stop recording, or you can do it through file --> stop recording.
5. After you finished recording or tracking your gameplay, you can go ahead and export the data from settings. You do this by going to file->settings, path settings, and click the export data button.
6. After clicking the button, type in your minecraft username, click ok and it should export the tracked stat data as a csv file and saved it to the specified directory for csv stat files. You can change this directory to whatever you want. The same goes for recordings as well.

For a more visual demonstration, please feel free to watch the demo video on YouTube, linked below. If anyone has any questions about the setup or usage processes, please feel free to create an issue on this Github repo and I will respond as soon as I am able to
## Application Demo Video
I uploaded a demo video of how the application works along with all of its functions up on YouTube. You can view it [here](https://www.youtube.com/watch?v=iPtLKEDAJ4k).

## Application Screenshots

1. Main Window:
<img width="1900" height="1022" alt="image" src="https://github.com/user-attachments/assets/0745f770-df16-4614-84de-becca4aecfaa" />

2. Settings - General
<img width="781" height="651" alt="image" src="https://github.com/user-attachments/assets/42517edc-d044-4d9b-9e83-240fe8e4800f" />

3. Settings - Paths
<img width="768" height="652" alt="image" src="https://github.com/user-attachments/assets/79cceb99-b407-4636-9f93-ba413ab7f810" />

4. Settings - DB Settings
<img width="762" height="647" alt="image" src="https://github.com/user-attachments/assets/4d30ad0a-3799-4440-80fe-bdb47bcfe4e5" />

5. About Page
<img width="648" height="555" alt="image" src="https://github.com/user-attachments/assets/1a2fc3f8-7d26-4934-b6d9-616308ba5faf" />

## Screenshots of Results
1. Screen Recording
<img width="1918" height="1017" alt="SS 6" src="https://github.com/user-attachments/assets/41458935-6e1f-4783-b31d-9f42b0bae130" />

2. Exported Data Spreadsheet (.CSV File)
<img width="1916" height="987" alt="SS 7" src="https://github.com/user-attachments/assets/8fea4557-b740-44fb-ab36-45de74c0d643" />

3. Expoted Data Stats - DB View (via PostgreSQL)
<img width="1497" height="636" alt="SS 8" src="https://github.com/user-attachments/assets/247fc136-6eb2-4abe-8065-b6a7319ac65d" />

## Support The Development
If you enjoyed using my program, please feel free to give me a donation. It really helps to motivate me into putting more features into the application. If interested, you can go ahead and give me donation using the following link:
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W21THQ12)



 

