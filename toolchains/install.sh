#!/bin/bash

DSP_LIC_FILE="158x_float.lic"
DSP_LICSERV_PORT=6677
SDK_ROOT="${PWD}/bta_sdk"
XTENSA_ROOT=${HOME}/airoha_sdk_toolchain

CADENCE_LIC_HOST=lic.airoha.com.tw
CADENCE_LIC_URL=${CADENCE_LIC_HOST}/all_key_gen.sh?float_req_hostid=
INSTALL_WORK_DIR="${PWD}"
XTENSA_PATH="${XTENSA_ROOT}/xtensa"
XTENSA_VERSION_2021="RI-2021.8-linux"
XTENSA_BIN_PATH="${XTENSA_ROOT}/xtensa/${XTENSA_VERSION_2021}/XtensaTools/bin"
SDK_PACKAGE="IoT_SDK_for_BT_Audio_V3.2.0.AB1565_AB1568.7z"

DSP_TOOL_PACKAGE_2021="XtensaTools_RI_2021_8_linux.tgz"
DSP_CONFIG_2021="AB1568_i64B_d32B_512K_linux_redist.tgz"
DSP_CONFIG_2021_INSTALLED="no"
DSP_TOOL_FOLDER=${INSTALL_WORK_DIR}
DSP_LICSERV_FOLDER="${XTENSA_ROOT}/xt_server"
SEVEN_ZA="7z"
DSP_LICSERV_PACKAGE=licserv_linux_x64_v11_15.tgz

LMUTIL="${DSP_LICSERV_FOLDER}/x64_lsb/lmutil"
echo "Airoha IoT SDK for BT-Audio and Linux Tools all in one package installation"
echo "System requirements"
echo "   - Ubuntu 18.10"
echo ""
echo "Requirements during installation (for install required Linux component and get dsp tool chain license):"
echo "   - Network connection"
echo "   - Root permission"
echo ""
echo ""
echo "Getting required Linux component"
sudo apt-get update
sudo apt-get -y install p7zip-full
sudo apt-get -y install build-essential
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get -y install libc6-i386 lsb
sudo apt-get -y install curl

#Decompress license server

echo "'"
echo Decompress license server
if [ ! -e $DSP_LICSERV_PACKAGE ]; then
	echo "Error: please get SDK package $DSP_LICSERV_PACKAGE, and put in folder ${INSTALL_WORK_DIR}" 
	exit 1
fi

if [ ! -e "${DSP_LICSERV_FOLDER}" ]; then
        mkdir -p "${DSP_LICSERV_FOLDER}"
fi

if [ ! -e "/usr/tmp" ]; then
        sudo mkdir -p /usr/tmp
        sudo chmod a+w /usr/tmp
fi

echo DSP_TOOL_FOLDER
cd "$DSP_TOOL_FOLDER"
echo "$PWD"
tar -C "${DSP_LICSERV_FOLDER}" -zxvf "$DSP_LICSERV_PACKAGE"

HOST_MAC=$(${LMUTIL} lmhostid|grep -i "The FlexNet"| sed "s/The FlexNet host ID of this machine is \"\"*\(.*\)\"\"*/\1/i"|awk '{print $1}');

cd ${INSTALL_WORK_DIR}
#gen license file
if [ ${CADENCE_LIC_HOST} ]; then
	curl ${CADENCE_LIC_URL}${HOST_MAC} > ${INSTALL_WORK_DIR}/158x_float.lic
	if [ "$?" -ne "0" ]; then
    echo  "Get license fail, please check network setting, installation stop"
    exit 1
  fi
  echo "Remote request Cadence license success."  		
else 
	echo "Use user assigned license file"

fi	

if [ ! -e "${DSP_CONFIG_2021}" ]; then
	echo "Error: please get DSP config ${DSP_CONFIG_2021}, and put in folder ${INSTALL_WORK_DIR}" 
	exit 1
fi

if [ ! -e "${DSP_LIC_FILE}" ]; then
	echo "Error: please get DSP license by JIRA request and change the value of \${DSP_LIC_FILE}, and put the license file in folder ${INSTALL_WORK_DIR}" 
	exit 1
fi


if [ ! -e "$SDK_ROOT" ]; then
        mkdir -p "${SDK_ROOT}"
fi

if [ ! -e "$XTENSA_ROOT" ]; then
        mkdir -p "${XTENSA_ROOT}"
fi

if [ ! -e "${XTENSA_PATH}" ]; then
        mkdir -p "${XTENSA_PATH}"
fi

echo "'"
echo Decompress tool chain

if [ -d "${XTENSA_PATH}/${XTENSA_VERSION_2021}/" ]; then
   echo  "${XTENSA_PATH}/${XTENSA_VERSION_2021}/ is installed."
else
   #echo tar -C ${XTENSA_PATH} -zxvf ${DSP_TOOL_PACKAGE_2021} >> ${XTENSA_ROOT}/install_log
   #tar -C "${XTENSA_PATH}" -zxvf "${DSP_TOOL_PACKAGE_2021}"
   echo cp -r  ${DSP_TOOL_PACKAGE_2021} ${XTENSA_PATH}
   #cp -r myfolder /path/to/
   cp -r  ./RI-2021.8-linux ${XTENSA_PATH}
   tar -zxvf ./libgecodeint.tar.gz
   cp ./libgecodeint.a ${XTENSA_PATH}/RI-2021.8-linux/XtensaTools/lib
   
   if [ "$?" -ne "0" ]; then
           echo  "Error: decompress ${DSP_TOOL_PACKAGE_2021} fail."
           exit 1
   fi
fi

if [ -d "${XTENSA_PATH}/${XTENSA_VERSION_2021}/AB1568_i64B_d32B_512K/" ]; then
    DSP_CONFIG_2021_INSTALLED="yes"
else
    echo "tar -C ${XTENSA_PATH} -zxvf ${DSP_CONFIG_2021}"
    tar -C "${XTENSA_PATH}" -zxvf "${DSP_CONFIG_2021}"
    if [ "$?" -ne "0" ]; then
            echo  "Error: decompress ${DSP_CONFIG_2021} fail."
            exit 1
    fi
fi


if [ "$DSP_CONFIG_2021_INSTALLED" == "no" ]; then
     echo Install dsp config, may take few seconds, please wait...
     echo "Install dsp config, may take few seconds, please wait... " > ${XTENSA_ROOT}/install_log
     echo "cd ${XTENSA_PATH}/${XTENSA_VERSION_2021}/AB1568_i64B_d32B_512K"
     echo "./install --xtensa-tools ${XTENSA_ROOT}/xtensa/${XTENSA_VERSION_2021}/XtensaTools --no-default"
     cd "${DSP_LICSERV_FOLDER}"
     ls
     echo "0"
     cd "${XTENSA_ROOT}"
     ls
     echo "0"
     cd "${XTENSA_PATH}"
     ls
     echo "1"
     cd "${XTENSA_PATH}/${XTENSA_VERSION_2021}"
     ls
     echo "2"
     cd "${XTENSA_PATH}/${XTENSA_VERSION_2021}/AB1568_i64B_d32B_512K"
     ls
     echo "${XTENSA_ROOT}/xtensa/${XTENSA_VERSION_2021}/XtensaTools"
     sudo bash ./install --xtensa-tools "${XTENSA_ROOT}/xtensa/${XTENSA_VERSION_2021}/XtensaTools" --no-default
     if [ "$?" -ne "0" ]; then
             echo  "Error: install dsp package fail. The config may already installed or have incorrect tool path"
             echo  ""
             echo  "Something went wrong. Please remove folder ${XTENSA_ROOT} and execute ./install.sh under msys2 terminal again."
             exit 1;
     fi
else
    echo  "Dsp package was installed."
fi


echo ""
cd "${DSP_LICSERV_FOLDER}"
cp "${INSTALL_WORK_DIR}/${DSP_LIC_FILE}" x64_lsb
sed -i "s/<SERVERNAME>/${HOSTNAME}/" "${DSP_LICSERV_FOLDER}/x64_lsb/${DSP_LIC_FILE}"
sed -i "s/<PORT>/$DSP_LICSERV_PORT/g" "${DSP_LICSERV_FOLDER}/x64_lsb/${DSP_LIC_FILE}"
sed -i "s/<\/PATH\/TO\/XTENSAD>/xtensad/" "${DSP_LICSERV_FOLDER}/x64_lsb/${DSP_LIC_FILE}" 
echo '#!/bin/bash' > "${INSTALL_WORK_DIR}"/start_lic_server.sh
echo "lmgr_pid=\$(ps aux|awk '/lmgrd -c/ {print \$2}'); kill -9 \$lmgr_pid 2>/dev/null" >> "${INSTALL_WORK_DIR}"/start_lic_server.sh
echo "echo Please wait for a second for cleanup cadence license server network port..." >> "${INSTALL_WORK_DIR}"/start_lic_server.sh
echo "echo Staring the cadence license server" >> "${INSTALL_WORK_DIR}"/start_lic_server.sh
echo "sleep 3" >>${INSTALL_WORK_DIR}/start_lic_server.sh >> "${INSTALL_WORK_DIR}"/start_lic_server.sh
echo ${DSP_LICSERV_FOLDER}/x64_lsb/lmgrd -c \"${DSP_LICSERV_FOLDER}/x64_lsb/${DSP_LIC_FILE}\" -l log.txt >> "${INSTALL_WORK_DIR}"/start_lic_server.sh
echo "echo The cadence license server launched" >> "${INSTALL_WORK_DIR}"/start_lic_server.sh
echo "echo The cadence license server may take 1 min to take effect, you can check the log file: " ${XTENSA_ROOT}/log.txt "for detail running status" >> "${INSTALL_WORK_DIR}"/start_lic_server.sh
chmod a+x "${INSTALL_WORK_DIR}"/start_lic_server.sh

cp -f "${INSTALL_WORK_DIR}/start_lic_server.sh" ${XTENSA_ROOT}/

echo ""
echo Installing candence license server
"${INSTALL_WORK_DIR}"/start_lic_server.sh

#echo "'"
#echo Decompress SDK pakcage
#cd "${INSTALL_WORK_DIR}"
#${SEVEN_ZA} x -y "${SDK_PACKAGE}" -o"${SDK_ROOT}"
#if [ "$?" -ne "0" ]; then
#        echo  "Error: decompress ${SDK_PACKAGE} fail."
#        exit 1
#fi

#  ~/.airoha_sdk_env
echo "#### Auto generate by install script, do not modify" > ${XTENSA_ROOT}/airoha_sdk_env_158x
echo "SDK_PACKAGE := yes" >> ${XTENSA_ROOT}/airoha_sdk_env_158x
echo "XTENSA_SYSTEM := ${XTENSA_PATH}/${XTENSA_VERSION_2021}/XtensaTools/config/" >> ${XTENSA_ROOT}/airoha_sdk_env_158x
echo "XTENSA_LICENSE_FILE := $DSP_LICSERV_PORT@$HOSTNAME" >> ${XTENSA_ROOT}/airoha_sdk_env_158x
echo "XTENSA_ROOT := ${XTENSA_ROOT}" >> ${XTENSA_ROOT}/airoha_sdk_env_158x
echo "XTENSA_BIN_PATH := ${XTENSA_BIN_PATH}" >> ${XTENSA_ROOT}/airoha_sdk_env_158x

sleep 5 
echo ""
echo ""
echo ""
echo "Installation done." 
echo "!!!!! Please remember to run '~/airoha_sdk_toolchain/start_lic_server' again if you reboot the system !!!!!"
echo "Now you can start the first build, please change directory to bta_sdk and execute build command."
echo "Example: " 
echo "       cd bta_sdk"
echo "       ./build.sh ab1565_8m_evk earbuds_ref_design"

