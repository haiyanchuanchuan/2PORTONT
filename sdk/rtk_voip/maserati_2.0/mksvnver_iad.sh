#!/bin/sh
# Filename: mksvnver_iad.sh
# Purpose : Append Global SVN version behind SDK version
#           This script create/modify linux-2.6.x/rtk_voip/include/svn_version.h.
# Fox example
# 	VOIP_VER : 8.6.3
#	VOIP_SYS_REV : 5899 (system SVN)
#	VOIP_LIX_REV : 5891 (linux  SVN)
#	VOIP_USR_REV : 5895 (user   SVN)
#	VOIP_VERSION will be : 8.6.3-R5899    (Realtek)
#
# Param $1 = root folder for svn info
#
# Auther : vincent.fann 2010/07/15
#

SDK_VER_H="$1/user/goahead-2.1.1/LINUX/version.c"
VOIP_LINUX_VER_H="$1/linux-2.6.x/rtk_voip/include/voip_version.h" 
VOIP_SOLAR_CFG="$1/user/rtk_voip/solar.conf" 

LINIX_SVN_VER_H="$1/linux-2.6.x/rtk_voip/include/svn_version.h" 
USERS_SVN_VER_H="$1/user/rtk_voip/includes/svn_version.h" 

VOIP_SYS_DIR="$1"
VOIP_LIX_DIR="$1/linux-2.6.x/rtk_voip"
VOIP_USR_DIR="$1/user/rtk_voip"

DELIMITER="-R"

if [ ! -d $1/.svn ]; then
	echo "SVN info not found" 
	exit 0
fi

# Get Linux SYSTEM version
if [ -f $SDK_VER_H ]; then 
	SYS_VER=`cat $SDK_VER_H | grep "#define SDK_VERSION" | cut -f2 -d "\"" | cut -f3 -d" "`
else
    SYS_VER="IAD"
fi

#Remove leading char 'v' from v2.5
#SYS_VER=`echo $SYS_VER | sed -e 's/^[A-Za-z]//'`
#echo SYS_VER:$SYS_VER

# Get current linux VoIP version from voip_version.h
VOIP_VER=`cat $VOIP_LINUX_VER_H | grep "#define SDK_VERSION" | cut -f2 -d "\""`
#echo $VOIP_VER

# Get current solar version 
VOIP_USR_CM_VER=`cat $VOIP_SOLAR_CFG | grep "user_agent_ver=" | cut -f2 -d "="`
#echo $VOIP_USR_CM_VER

# Get SYS SVN subversion
cd $VOIP_LIX_DIR
VOIP_LIX_REV=`svn info | grep "Last Changed Rev" | cut -f4 -d" "`
#echo LIX:$VOIP_LIX_REV
cd - &> /dev/null

# Get USR SVN subversion
cd $VOIP_USR_DIR
VOIP_USR_REV=`svn info | grep "Last Changed Rev" | cut -f4 -d" "`
#echo USR:$VOIP_USR_REV
cd - &> /dev/null

# Get SYS SVN subversion
cd $1
VOIP_SYS_REV=`svn info | grep "Last Changed Rev" | cut -f4 -d" "`
#echo SYS:$VOIP_SYS_REV
cd - &> /dev/null

# Always use MAX SVN number
# Find MAX REV number
MAX_SVN_REV="$VOIP_SYS_REV"

if [ "$VOIP_USR_REV" -gt "$MAX_SVN_REV" ]; then
	MAX_SVN_REV="$VOIP_USR_REV"
fi

if [ "$VOIP_LIX_REV" -gt "$MAX_SVN_REV" ]; then
	MAX_SVN_REV="$VOIP_LIX_REV"
fi

# Append sub version to SDK version 

if [ "$VOIP_USR_CM_VER" == "" ]; then
	VOIP_FULL="$SYS_VER-$VOIP_VER$DELIMITER$MAX_SVN_REV"
else
	VOIP_FULL="$SYS_VER-$VOIP_VER-$VOIP_USR_CM_VER$DELIMITER$MAX_SVN_REV"
fi

# Check current version
if [ -f $LINIX_SVN_VER_H ]; then 
	ORG_SDK_VER=`cat $LINIX_SVN_VER_H | grep "#define VOIP_VERSION" | cut -f2 -d "\""`
	
	#echo OLD REV: $ORG_SDK_VER
	#echo NEW REV: $VOIP_FULL

	# Only append when we have a new SVN version
	# DO NOT update it if they are the same. Saving compile time.
	if [ "$VOIP_FULL" == "$ORG_SDK_VER" ]; then
		echo "VOIP version $VOIP_FULL"
		exit 0;
	fi
else
	touch $LINIX_SVN_VER_H
fi

echo "#define VOIP_VERSION   \"$VOIP_FULL\"" 	>  $LINIX_SVN_VER_H
echo "#define VOIP_SYS_REV   \"$VOIP_SYS_REV\"" >> $LINIX_SVN_VER_H
echo "#define VOIP_USR_REV   \"$VOIP_USR_REV\"" >> $LINIX_SVN_VER_H
echo "#define VOIP_LIX_REV   \"$VOIP_LIX_REV\"" >> $LINIX_SVN_VER_H

echo "#define VOIP_VERSION   \"$VOIP_FULL\""    > $USERS_SVN_VER_H
echo "#define VOIP_SYS_REV   \"$VOIP_SYS_REV\"" >> $USERS_SVN_VER_H
echo "#define VOIP_USR_REV   \"$VOIP_USR_REV\"" >> $USERS_SVN_VER_H
echo "#define VOIP_LIX_REV   \"$VOIP_LIX_REV\"" >> $USERS_SVN_VER_H

echo "VOIP version $VOIP_FULL"
