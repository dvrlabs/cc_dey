# Directory structure
# ./ccapi
#   |
#   |-ccapi
#   |  |-include
#   |  |  |-ccapi
#   |  |  |-ccimp
#   |  |-source
#   |     |-ccfsm
#   |        |-include
#   |        |-source
#   |-ccimp
#   |-demo
#
TARBALL_NAME=ccapi
OUTPUT_DIR=output
OUTPUT_CCAPI_DIR=$OUTPUT_DIR/ccapi
OUTPUT_CCIMP_DIR=$OUTPUT_DIR/ccimp
OUTPUT_CCFSM_DIR=$OUTPUT_CCAPI_DIR/source/ccfsm
OUTPUT_DEMO_DIR=$OUTPUT_DIR/demo

LOCAL_CCAPI_DIR=source
LOCAL_CCFSM_DIR=source/cc_ansic
LOCAL_CCIMP_DIR=tests/ccimp
LOCAL_DEMO_DIR=tests/samples/demo

rm -rf $OUTPUT_DIR

mkdir $OUTPUT_DIR
mkdir $OUTPUT_CCAPI_DIR
mkdir $OUTPUT_CCIMP_DIR
mkdir $OUTPUT_CCAPI_DIR/source
mkdir $OUTPUT_CCAPI_DIR/include
mkdir $OUTPUT_CCAPI_DIR/include/ccapi
mkdir $OUTPUT_CCAPI_DIR/include/ccimp
mkdir $OUTPUT_CCAPI_DIR/include/custom

mkdir $OUTPUT_CCFSM_DIR
mkdir $OUTPUT_CCFSM_DIR/source
mkdir $OUTPUT_CCFSM_DIR/include

cp -r $LOCAL_CCAPI_DIR/*.[ch] $OUTPUT_CCAPI_DIR/source
cp -r $LOCAL_CCFSM_DIR/public/include/*.h $OUTPUT_CCFSM_DIR/include
cp -r $LOCAL_CCFSM_DIR/private/*.[ch] $OUTPUT_CCFSM_DIR/source

cp -r $LOCAL_CCIMP_DIR/*.[ch] $OUTPUT_CCIMP_DIR
cp include/ccapi/*.h $OUTPUT_CCAPI_DIR/include/ccapi
cp include/ccimp/*.h $OUTPUT_CCAPI_DIR/include/ccimp
cp include/custom/*.h $OUTPUT_CCAPI_DIR/include/custom

# Replace custom headers
cp $LOCAL_CCAPI_DIR/cc_ansic_custom_include/*.h $OUTPUT_CCFSM_DIR/include

mkdir $OUTPUT_DEMO_DIR
cp $LOCAL_DEMO_DIR/*.[ch] $OUTPUT_DEMO_DIR
cp $LOCAL_DEMO_DIR/Makefile_for_demo $OUTPUT_DEMO_DIR/Makefile

# Deactivate RCI service and Firmware Update
sed -i -e 's/#define CCIMP_RCI_SERVICE_ENABLED/#undef CCIMP_RCI_SERVICE_ENABLED/' $OUTPUT_CCAPI_DIR/include/custom/custom_connector_config.h

# Uncomment errors and leave default MAC and Vendor ID
sed -i '/#define MAC_ADDRESS_STRING/c\#define MAC_ADDRESS_STRING      "112233:445566"' $OUTPUT_DEMO_DIR/main.c
sed -i '/#define VENDOR_ID/c\#define VENDOR_ID               0x00000000' $OUTPUT_DEMO_DIR/main.c
sed -i -e 's/\/\/#error/#error/' $OUTPUT_DEMO_DIR/main.c

for FILE in $(find ./${OUTPUT_DIR} -name \*.[ch] -not -path "./${OUTPUT_DIR}/ccapi/source/ccfsm/*")
do
    cat copyright_header.txt | cat - $FILE > /tmp/file && mv /tmp/file $FILE
done

echo "Creating the Tarball ${OUTPUT_DIR}/${TARBALL_NAME}.tgz."
mv $OUTPUT_DIR ccapi
tar -czvf "${TARBALL_NAME}.tgz" "ccapi"
rm -rf ccapi
