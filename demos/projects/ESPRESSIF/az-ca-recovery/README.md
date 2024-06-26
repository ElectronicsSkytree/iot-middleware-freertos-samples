# Perform Trust Bundle Recovery with an ESPRESSIF ESP32 using Azure IoT Middleware for FreeRTOS

Trust Bundle Recovery allows a mechanism for your device to recover connection to operational Azure resources should the TLS CA certificates expire or otherwise become invalid. This requires an additional, separate "recovery" DPS instance and device credential, only to be used for recovery of the CA certificates. Should CA verification fail while connecting to operational IoT services, the device should connect to the recovery DPS instance, ignoring the TLS CA cert validation, and will be returned a signed CA trust bundle recovery payload from the recovery DPS. This payload contains the complete and updated collection of the current CA certificates, asymmetrically signed by a root key created by the user. The device receives this payload, compares the signature using the saved public key, and then after successful validation, installs the certificates into the device's non-volatile storage (NVS). This permanent storage allows the new certificates to be loaded again should the device restart, removing the need for the device to reconnect to the recovery endpoint again. From that point on, the device can connect to the operational DPS endpoint and provisioned hub with full CA trust validation enabled.

## IMPORTANT SECURITY WARNINGS

**NOTE**: The information sent and received between the device and your recovery DPS instance should not be assumed to be completely secure. The connection is made with TLS and is encrypted, but because the server CA cert is not checked, you do expose yourself to a potential man-in-the-middle (MITM) attack. The risk of malicious payloads being received is mitigated by signing the payload which the device verifies. Do not send confidential information in that connection, and do not trust any received data unless it is signed by your recovery root keys.

**NOTE**: This sample does not use NVS encryption. It is advised to use NVS encryption so that write operations to the NVS trust bundle section are performed only by those authorized to. For more information on implementing this, [please see here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html#security-tampering-and-robustness).

## Sequence Diagram

```mermaid
sequenceDiagram
    autonumber
    participant Device
    participant DPS_operational
    participant DPS_recovery
    participant CustomAllocation

    Note right of CustomAllocation: .NET Azure Function provisioned with a RSA<br> certificate stored within the Certificate Store:<br>-RSASign_Pub(N, e)<br>-RSASign_Pri

    Note right of Device: Device firmware additions:<br>-TrustBundle,   version=1<br>-RSASign_Pub(N, e)<br>-DPS_Recovery Identity (IDScope, registrationID, SharedAccessKey)

    Device-->>DPS_operational: Connect
    Note right of DPS_operational: To reduce the number of recovery attempts<br> for devices that may encounter <br>a WiFi Captive Portal, we recommend<br> retrying prior to initiating recovery.

    DPS_operational-->>Device: Invalid CA Certificate

    Note left of Device: Initiate Recovery
    Device-->>DPS_recovery: Connect using DPS_Recovery Identity
    Note right of Device: !! Ignore CA validation !!
    Note right of Device: !! Important !!: the credentials, device name, etc.<br> may be recorded by a man-in-the-middle observer.


    DPS_recovery-->>CustomAllocation: Recovery Identity
    CustomAllocation-->>CustomAllocation: Verify device CA TrustBundle version
    CustomAllocation-->>DPS_recovery: Custom Payload
    Note left of CustomAllocation:1. Payload: -CA TrustBundle, new_version=2<br>-Device registrationID<br>-expiryTime=NOW + 2min<br><br>2.RSA_Sign(data=Payload, key=RSASign_Pri)
    DPS_recovery-->>Device: Registration Information (including Custom Payload)
    Device-->>Device: RSA_Verify(data=Payload, key=RSASign_Pub),<br> new_version > version and expiryTime > NOW
    Device-->>Device: Update CA TrustBundle
    Device-->>Device: Update TrustBundle version
    Note left of Device: End Recovery
    Device-->>DPS_operational: Connect
    DPS_operational-->>Device: Valid CA Certificate
    Device-->>DPS_operational: Resume normal operation.
```

## What you need

- [ESPRESSIF ESP32 Board](https://www.espressif.com/en/products/devkits)
- Wi-Fi 2.4 GHz
- USB 2.0 A male to Micro USB male data cable
- [ESP-IDF](https://idf.espressif.com/): This guide was tested against ESP-IDF [v4.4.3](https://github.com/espressif/esp-idf/tree/v4.4.3) and [v5.0](https://github.com/espressif/esp-idf/tree/v5.0).
- To run this sample we will use the Device Provisioning Service to provision your device automatically. **Note** that even when using DPS, you still need an IoT Hub created and connected to DPS.

  | DPS |
  |---------- |
  | Have an instance of [IoT Hub Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/quick-setup-auto-provision#create-a-new-iot-hub-device-provisioning-service) |
  | Have an [individual enrollment](https://docs.microsoft.com/azure/iot-dps/how-to-manage-enrollments#create-a-device-enrollment) created in your instance of DPS using your preferred authentication method* |

## Install prerequisites

1. GIT

    Install `git` following the [official website](https://git-scm.com/).

1. ESP-IDF

    On Windows, install the ESPRESSIF ESP-IDF using this [download link](https://dl.espressif.com/dl/esp-idf).

    For other Operating Systems or to update an existing installation, follow [Espressif official documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#get-started).

1. The latest version of Azure CLI and Azure IoT Module

    See steps to install both [here](https://learn.microsoft.com/azure/iot-hub-device-update/create-update?source=recommendations#prerequisites).

1. Azure IoT Embedded middleware for FreeRTOS

1. OpenSSL command line tools

Clone the following repo to download all sample device code, setup scripts, and offline versions of the documentation.

**If you previously cloned this repo in another sample, you don't need to do it again.**

```bash
git clone https://github.com/Azure-Samples/iot-middleware-freertos-samples.git
```

To initialize the repo, run the following commands:

```bash
cd iot-middleware-freertos-samples
git submodule update --init --recursive
```

You may also need to enable long path support for both Microsoft Windows and git:

- Windows: <https://docs.microsoft.com/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd#enable-long-paths-in-windows-10-version-1607-and-later>
- Git: as Administrator run `git config --system core.longpaths true`

## Set Up Azure Resources

In order to create the resources to use for the recovery process, we are going to use an Azure Resource Manager (ARM) template. The file detailing all the resources to be deployed is located in [demos/sample_azure_iot_ca_recovery/ca-recovery-template.bicep](../../../sample_azure_iot_ca_recovery/ca-recovery-template.bicep). You will login to the Azure CLI, and then deploy the resources to a subscription and resource group of your choosing.

### Deploy Resources

With the Azure CLI installed, navigate to the `demos/sample_azure_iot_ca_recovery` directory.

From there, run the following

```bash

# For any < > value, substitute in the value of your choice.
az login
az account set --subscription <subscription id>

# To see a list of locations, run the following and look for `name` values.
az account list-locations | ConvertFrom-Json | format-table -Property name

# Create a resource group
az group create --name '<name>' --location '<location>'

# Deploy the resources. This may take a couple minutes.
#  - `resourcePrefix`: choose a unique, short name with only lower-case letters which
#                      will be prepended to all of the deployed resources.
#  - `name`: choose a name to give this deployment, between 3 and 24 characters in length and
#            use numbers and lower-case letters only.
az deployment group create --name '<deployment name>' --resource-group '<name>' --template-file './ca-recovery-arm.bicep' --parameters location='<location>' resourcePrefix='<your prefix>'
```

### Create and Import Signing Certificate

Generate the certificate which will be used to sign the recovery payload.

```powershell
openssl genrsa -out recovery-private-key.pem 2048
openssl req -new -key recovery-private-key.pem -x509 -days 36500 -out recovery-public-cert.pem

# You might not need the -legacy flag.
# Make sure to add a password to your certificate which will be needed when importing it.
openssl pkcs12 -export -in recovery-public-cert.pem -inkey recovery-private-key.pem -out recovery-key-pairs.pfx -legacy
```

Find the Azure Function App which was deployed in your resource group. Under `Settings`, we will add the certificate (`recovery-key-pairs.pfx`) to the `Certificates` section.

![img](./images/AzureFunctionCertificate.png)

Make sure to import the password which you may have added to the certificate.

![img](./images/AzureFunctionCertificateImport.png)

Once that is imported, find the thumbprint for the certificate.

![img](./images/AzureFunctionCertificateThumbprint.png)

Make note or copy the thumbprint. Create an application setting for the function called `CERT_THUMBPRINT` with the thumbprint as the value. Click Save.

![img](./images/AzureFunctionCertificateConfig.png)

### Create Enrollment Group with Custom Allocation

Navigate to your DPS instance, select `Manage enrollments` on the left side, and select `Add enrollment group` to create a group enrollment. For authentication, use `Symmetric key`. Under `Select how your want to assign devices to hubs`, select `Custom (Use Azure Function)`.

![img](./images/AzureGroupEnrollment.png)

As you scroll down, select the Azure Function which was deployed previously.

Select `Save` and make note of the Group Enrollment Key.

## Create a Derived Shared Access Key for Group Enrollment

To use the custom allocation policy with the recovery Azure function, you have to create a derived SAS key from the group enrollment key. To create one, use the following Azure CLI command or [follow the directions here for other options](https://learn.microsoft.com/azure/iot-dps/concepts-symmetric-key-attestation?tabs=azure-cli#group-enrollments). You may use a registration id of your choice. Make sure to save this for the [RECOVERY] Derived Shared Access Key later. The `--key` parameter should be the group enrollment "Primary Key".

```bash
az iot dps enrollment-group compute-device-key --key "<key>" --registration-id "<registration id>"
```

## Prepare the TLS CA Trust Bundle in the NVS

[Follow the README linked here to run the sample called az-nvs-cert-bundle](../az-nvs-cert-bundle/README.md) in the `demos/projects/ESPRESSIF` directory to load the version 1 trust bundle in your ESP device. This will purposely save an incomplete trust bundle in your devices's NVS, which will then be loaded for the `az-ca-recovery` application. Once the CA validation fails in the `az-ca-recovery` sample, the device will then move into the recovery phase which fetches the new and complete certificate trust bundle.

## Prepare the sample

After running the set up application, you will then need to update this sample configuration, build the image, and flash the image to the device.

### Checklist

At this point, you should have the following services and values:

| Service | Details |
| --- | --- |
| **[OPERATIONAL]** Device Provisioning Service | - You should have this already created, with an individual enrollment.<br>- Have available:<br>&nbsp;&nbsp;- Registration ID<br>&nbsp;&nbsp;- ID Scope<br>&nbsp;&nbsp;- Shared Access Key |
| **[OPERATIONAL]** IoT Hub | - Make sure it is connected to Operational Device Provisioning Service. |
| **[RECOVERY]** Device Provisioning Service | - This was deployed previously by the ARM template.<br>- Have available:<br>&nbsp;&nbsp;- Registration ID<br>&nbsp;&nbsp;- ID Scope<br>&nbsp;&nbsp;- Derived Shared Access Key |
| **[RECOVERY]** IoT Hub | - Deployed previously by the ARM template.<br>- Device will not connect to this. Only there to satisfy the Device Provisioning Service.<br>- Nothing to do. |
| **[RECOVERY]** Azure Function | - Have the certificate uploaded to the `Certificates` section.<br>- Add the certificate thumbprint to the function's `Configuration` section. |
| - **[RECOVERY]** Application Insights<br>- **[RECOVERY]** Storage account<br>- **[RECOVERY]** App Service plan | - Nothing to do. Deployed by the ARM template. |


### Update sample configuration

The configuration of the ESPRESSIF ESP32 sample uses ESP-IDF' samples standard [kconfig](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html) configuration.

On a [console with ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#step-4-set-up-the-environment-variables), navigate to the ESP32 `az-ca-recovery` project directory: `demos\projects\ESPRESSIF\az-ca-recovery` and run the following commands:

```shell
idf.py menuconfig
```

#### Ignore CA Cert Validation

Under menu item `Component config`, go to `ESP-TLS`, check the `Allow potentially insecure options` option and then select the "Skip server certificate validation" option there. Hit the `Esc` key two times to return back to the top level.

#### Azure IoT Values

Under menu item `Azure IoT middleware for FreeRTOS Sample Configuration`, update the following configuration values:

Parameter | Value
---------|----------
 `WiFi SSID` | _{Your WiFi SSID}_
 `WiFi Password` | _{Your WiFi password}_

Under menu item `Azure IoT middleware for FreeRTOS Main Task Configuration`, update the following configuration values:

Parameter | Value
---------|----------
 `Enable Device Provisioning Sample` | _{Check this option to enable DPS in the sample}_
 `[OPERATIONAL] Azure Device Provisioning Service ID Scope` | _{Your ID scope value}_
 `[OPERATIONAL] Azure Device Provisioning Service Registration ID` | _{Your Device Registration ID value}_
 `[RECOVERY] Azure Device Provisioning Service ID Scope` | _{Your ID scope value for the recovery instance}_
 `[RECOVERY] Azure Device Provisioning Service Registration ID.` | _{Your Device Registration ID value for the recovery instance}_

Select your desired authentication method with the `Azure IoT Authentication Method () --->` as `Symmetric Key`:

Parameter | Value
---------|----------
 `[OPERATIONAL] Azure IoT Device Symmetric Key` | _{Your Device Provisioning device symmetric key for the operational DPS instance}_
 `[RECOVERY] Azure IoT Device Symmetric Key` | _{Your Device Provisioning device symmetric key for the recovery DPS instance}_

Save the configuration (`Shift + S`) inside the sample folder in a file with name `sdkconfig`.
After that, close the configuration utility (`Shift + Q`).

#### Signing Certificate

You must also update the signing root key which is located in `config/demo_config.h`, titled `ucAzureIoTRecoveryRootKeyN`, and  the exponent (E) value `ucAzureIoTRecoveryRootKeyE`. You can get the hex value of the modulus (N value) using the below command. **Note** that most times the exponent is defaulted to 65537 (0x10001). If that is the case, you may use `{ 0x01, 0x00, 0x01 }` for the exponent. Otherwise, see the below command to check your exponent value and format it appropriately.

For bash:

```bash
# Modulus
openssl x509 -in recovery-public-cert.pem -modulus -noout | sed s/Modulus=// | sed -r 's/../0x&, /g'

# Exponent
openssl x509 -in recovery-public-cert.pem -text -noout | grep Exponent | sed -r 's/.*Exponent: .*\((.*)\)/\1/g'
```

For Powershell:

```powershell
# Modulus
openssl x509 -in recovery-public-cert.pem -modulus -noout | ForEach-Object { $_ -replace 'Modulus=', '' } | ForEach-Object { $_ -replace '(..)', '0x$1, ' }

# Exponent
openssl x509 -in recovery-public-cert.pem -text -noout | Select-String Exponent |  ForEach-Object { $_ -replace 'Exponent: .*\((.*)\)', '$1' }
```

## Build the image

To build the device image, run the following command (the path `"C:\espbuild"` is only a suggestion, feel free to use a different one, as long as it is near your root directory, for a shorter path):

  ```bash
  idf.py --no-ccache -B "C:\espbuild" build
  ```

## Flash the image

1. Connect the Micro USB cable to the Micro USB port on the ESPRESSIF ESP32 board, and then connect it to your computer.

2. Find the COM port mapped for the device on your system.

    On **Windows** (and using powershell), run the following:

    ```powershell
    Get-WMIObject Win32_SerialPort | Select-Object Name,DeviceID,Description,PNPDeviceID
    ```

     Look for a device with `CP210x-` in the title. The COM port should be something similar to `COM5`.

    On **Linux**, run the following:

    ```shell
    ls -l /dev/serial/by-id/
    ```

    Look for a "CP2102"-based entry and take note of the path mapped for your device (e.g. "/dev/ttyUSB0").

3. Run the following command:

    > This step assumes you are in the ESPRESSIF ESP32 sample directory (same as configuration step above).

    ```bash
    idf.py --no-ccache -B "C:\espbuild" -p <COM port> flash
    ```

    <details>
    <summary>Example...</summary>

    On **Windows**:

    ```shell
    idf.py --no-ccache -B "C:\espbuild" -p COM5 flash
    ```

    On **Linux**:

    ```shell
    idf.py -p /dev/ttyUSB0 flash
    ```
    </details>

## Confirm device connection

You can use any terminal application to monitor the operation of the device and confirm it is set up correctly.

Alternatively you can use ESP-IDF monitor:

```bash
idf.py -B "C:\espbuild" -p <COM port> monitor
```

The output should show traces similar to:

<details>
<summary>See more...</summary>

```shell
$ idf.py -p /dev/ttyUSB0 monitor
Executing action: monitor
Running idf_monitor in directory /iot-middleware-freertos-samples-ew/demos/projects/ESPRESSIF/esp32
Executing "/home/user/.espressif/python_env/idf4.4_py3.8_env/bin/python /esp/esp-idf/tools/idf_monitor.py -p /dev/ttyUSB0 -b 115200 --toolchain-prefix xtensa-esp32-elf- --target esp32 --revision 0 /iot-middleware-freertos-samples-ew/demos/projects/ESPRESSIF/esp32/build/azure_iot_freertos_esp32.elf -m '/home/user/.espressif/python_env/idf4.4_py3.8_env/bin/python' '/esp/esp-idf/tools/idf.py' '-p' '/dev/ttyUSB0'"...
--- idf_monitor on /dev/ttyUSB0 115200 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
ets Jun  8 2016 00:22:57

rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:6744
load:0x40078000,len:14268
ho 0 tail 12 room 4
load:0x40080400,len:3716
0x40080400: _init at ??:?

entry 0x40080680
I (28) boot: ESP-IDF v4.4-dev-1853-g06c08a9d9 2nd stage bootloader
I (28) boot: compile time 18:49:13
I (28) boot: chip revision: 1
I (33) boot_comm: chip revision: 1, min. bootloader chip revision: 0
I (49) boot.esp32: SPI Speed      : 40MHz
I (49) boot.esp32: SPI Mode       : DIO
I (49) boot.esp32: SPI Flash Size : 2MB
I (53) boot: Enabling RNG early entropy source...
I (59) boot: Partition Table:
I (62) boot: ## Label            Usage          Type ST Offset   Length
I (70) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (77) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (85) boot:  2 factory          factory app      00 00 00010000 00100000
I (92) boot: End of partition table
I (96) boot_comm: chip revision: 1, min. application chip revision: 0
I (103) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=1d2c8h (119496) map
I (155) esp_image: segment 1: paddr=0002d2f0 vaddr=3ffb0000 size=02d28h ( 11560) load
I (160) esp_image: segment 2: paddr=00030020 vaddr=400d0020 size=89ce0h (564448) map
I (365) esp_image: segment 3: paddr=000b9d08 vaddr=3ffb2d28 size=00a5ch (  2652) load
I (367) esp_image: segment 4: paddr=000ba76c vaddr=40080000 size=1437ch ( 82812) load
I (405) esp_image: segment 5: paddr=000ceaf0 vaddr=50000000 size=00010h (    16) load
I (415) boot: Loaded app from partition at offset 0x10000
I (415) boot: Disabling RNG early entropy source...
I (427) cpu_start: Pro cpu up.
I (428) cpu_start: Starting app cpu, entry point is 0x4008113c
0x4008113c: call_start_cpu1 at /esp/esp-idf/components/esp_system/port/cpu_start.c:150

I (0) cpu_start: App cpu up.
I (442) cpu_start: Pro cpu start user code
I (442) cpu_start: cpu freq: 160000000
I (442) cpu_start: Application information:
I (446) cpu_start: Project name:     azure_iot_freertos_esp32
I (453) cpu_start: App version:      7ac616e-dirty
I (458) cpu_start: Compile time:     Jul 15 2021 22:32:07
I (464) cpu_start: ELF file SHA256:  965ee48390cd1e56...
I (470) cpu_start: ESP-IDF:          v4.4-dev-1853-g06c08a9d9
I (477) heap_init: Initializing. RAM available for dynamic allocation:
I (484) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (490) heap_init: At 3FFB8C48 len 000273B8 (156 KiB): DRAM
I (496) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (503) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (509) heap_init: At 4009437C len 0000BC84 (47 KiB): IRAM
I (516) spi_flash: detected chip: winbond
I (520) spi_flash: flash io: dio
W (524) spi_flash: Detected size(8192k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (538) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (732) wifi:wifi driver task: 3ffba728, prio:23, stack:6656, core=0
I (732) system_api: Base MAC address is not set
I (732) system_api: read default base MAC address from EFUSE
I (752) wifi:wifi firmware version: ff5f4ea
I (752) wifi:wifi certification version: v7.0
I (752) wifi:config NVS flash: enabled
I (752) wifi:config nano formating: disabled
I (762) wifi:Init data frame dynamic rx buffer num: 32
I (762) wifi:Init management frame dynamic rx buffer num: 32
I (772) wifi:Init management short buffer num: 32
I (772) wifi:Init dynamic tx buffer num: 32
I (782) wifi:Init static rx buffer size: 1600
I (782) wifi:Init static rx buffer num: 10
I (782) wifi:Init dynamic rx buffer num: 32
I (792) wifi_init: rx ba win: 6
I (792) wifi_init: tcpip mbox: 32
I (792) wifi_init: udp mbox: 6
I (802) wifi_init: tcp mbox: 6
I (802) wifi_init: tcp tx win: 5744
I (812) wifi_init: tcp rx win: 5744
I (812) wifi_init: tcp mss: 1440
I (812) wifi_init: WiFi IRAM OP enabled
I (822) wifi_init: WiFi RX IRAM OP enabled
I (822) example_connect: Connecting to ContosoWiFi...
I (832) phy_init: phy_version 4670,719f9f6,Feb 18 2021,17:07:07
I (932) wifi:mode : sta (84:cc:a8:4c:7e:fc)
I (932) wifi:enable tsf
I (942) example_connect: Waiting for IP(s)
I (2992) wifi:new:<11,0>, old:<1,0>, ap:<255,255>, sta:<11,0>, prof:1
I (2992) wifi:state: init -> auth (b0)
I (3002) wifi:state: auth -> assoc (0)
I (3012) wifi:state: assoc -> run (10)
I (3022) wifi:connected with ContosoWiFi, aid = 1, channel 11, BW20, bssid = 74:ac:b9:c1:39:76
I (3022) wifi:security: WPA2-PSK, phy: bgn, rssi: -61
I (3022) wifi:pm start, type: 1

I (3042) wifi:AP's beacon interval = 102400 us, DTIM period = 3
W (3042) wifi:<ba-add>idx:0 (ifx:0, 74:ac:b9:c1:39:76), tid:0, ssn:0, winSize:64
I (3612) esp_netif_handlers: example_connect: sta ip: 192.168.1.220, mask: 255.255.255.0, gw: 192.168.1.1
I (3612) example_connect: Got IPv4 event: Interface "example_connect: sta" address: 192.168.1.220
I (3622) example_connect: Connected to example_connect: sta
I (3622) example_connect: - IPv4 address: 192.168.1.220
[INFO] [AzureIoTDemo] [sample_azure_iot.c:580] Creating a TLS connection to contoso-iot-hub.azure-devices.net:8883.

I (6322) tls_freertos: (Network connection 0x3ffc8c4c) Connection to contoso-iot-hub.azure-devices.net established.
[INFO] [AzureIoTDemo] [sample_azure_iot.c:384] Creating an MQTT connection to contoso-iot-hub.azure-devices.net.
...
```
</details>

## Troubleshooting

- If you receive a message from DPS that the "Custom Allocation failed" with the return code of `400`, make sure you added the certificate thumbprint to the Azure Function ([see here for details](#create-and-import-signing-certificate)).
