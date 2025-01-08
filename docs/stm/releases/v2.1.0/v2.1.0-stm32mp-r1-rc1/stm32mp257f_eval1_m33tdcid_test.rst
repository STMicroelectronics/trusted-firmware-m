
##########################
stm32mp257f_eval1_m33tdcid
##########################


***********
Secure test
***********

.. tabs::

   .. group-tab:: Summary

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_IPC_TEST_1XXX
          - IPC secure interface test
          - PASSED

        * - TFM_S_PS_TEST_1XXX
          - PSA protected storage S interface tests
          - PASSED

        * - TFM_PS_TEST_3XXX
          - PS reliability tests
          - PASSED

        * - TFM_S_PS_TEST_3XXX
          - PS rollback protection tests
          - PASSED

        * - TFM_S_ITS_TEST_1XXX
          - PSA internal trusted storage S interface tests
          - PASSED

        * - TFM_ITS_TEST_2XXX
          - ITS reliability tests
          - PASSED

        * - TFM_S_CRYPTO_TEST_1XXX
          - Crypto secure interface tests
          - PASSED

        * - TFM_S_ATTEST_TEST_1XXX
          - Initial Attestation Service secure interface tests
          - PASSED

        * - TFM_S_PLATFORM_TEST_1XXX
          - Platform Service Secure interface tests
          - PASSED

   .. group-tab:: TFM_S_IPC_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_IPC_TEST_1001
          - Get PSA framework version
          - PASSED

        * - TFM_S_IPC_TEST_1002
          - Get version of an RoT Service
          - PASSED

        * - TFM_S_IPC_TEST_1004
          - Request connection-based RoT Service
          - PASSED

        * - TFM_S_IPC_TEST_1006
          - Call PSA RoT access APP RoT memory test service
          - PASSED

        * - TFM_S_IPC_TEST_1012
          - Request stateless service
          - PASSED

   .. group-tab:: TFM_S_PS_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_PS_TEST_1001
          - Set interface
          - PASSED

        * - TFM_S_PS_TEST_1002
          - Set interface with create flags
          - PASSED

        * - TFM_S_PS_TEST_1003
          - Set interface with NULL data pointer
          - PASSED

        * - TFM_S_PS_TEST_1005
          - Set interface with write once UID
          - PASSED

        * - TFM_S_PS_TEST_1006
          - Get interface with valid data
          - PASSED

        * - TFM_S_PS_TEST_1007
          - Get interface with zero data length
          - PASSED

        * - TFM_S_PS_TEST_1008
          - Get interface with invalid UIDs
          - PASSED

        * - TFM_S_PS_TEST_1009
          - Get interface with invalid data lengths and offsets
          - PASSED

        * - TFM_S_PS_TEST_1010
          - Get interface with NULL data pointer
          - PASSED

        * - TFM_S_PS_TEST_1011
          - Get info interface with write once UID
          - PASSED

        * - TFM_S_PS_TEST_1012
          - Get info interface with valid UID
          - PASSED

        * - TFM_S_PS_TEST_1013
          - Get info interface with invalid UIDs
          - PASSED

        * - TFM_S_PS_TEST_1015
          - Remove interface with valid UID
          - PASSED

        * - TFM_S_PS_TEST_1016
          - Remove interface with write once UID
          - PASSED

        * - TFM_S_PS_TEST_1017
          - Remove interface with invalid UID
          - PASSED

        * - TFM_S_PS_TEST_1018
          - Block compaction after remove
          - PASSED

        * - TFM_S_PS_TEST_1019
          - Multiple partial gets
          - PASSED

        * - TFM_S_PS_TEST_1020
          - Multiple sets to same UID from same thread
          - PASSED

        * - TFM_S_PS_TEST_1021
          - Get support interface
          - PASSED

        * - TFM_S_PS_TEST_1022
          - Set, get and remove interface with different asset sizes
          - PASSED

   .. group-tab:: TFM_PS_TEST_3XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_PS_TEST_2001
          - repetitive sets and gets in/from an asset
          - PASSED

        * - TFM_S_PS_TEST_2002
          - repetitive sets, gets and removes
          - PASSED

   .. group-tab:: TFM_S_PS_TEST_3XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_PS_TEST_3001
          - Check PS area version when NV counters 1/2/3 have the same value
          - PASSED

        * - TFM_S_PS_TEST_3002
          - Check PS area version when it is different from NV counters 1/2/3
          - PASSED

        * - TFM_S_PS_TEST_3003
          - Check PS area version when NV counters 1 and 2 are equals, 3 is different, and PS area version match NV counters 1 and 2
          - PASSED

        * - TFM_S_PS_TEST_3004
          - Check PS area version when NV counters 2 and 3 are equals, 1 is different and PS area version match NV counter 2 and 3
          - PASSED

        * - TFM_S_PS_TEST_3005
          - Check PS area version when NV counters 2 and 3 are equals, 1 is different and PS area version match NV counter 1
          - PASSED

        * - TFM_S_PS_TEST_3006
          - Check PS area version when NV counters 1, 2 and 3 have different values and PS area version match NV counter 1 value
          - PASSED

        * - TFM_S_PS_TEST_3007
          - Check PS area version when NV counters 1, 2 and 3 have different values and PS area version match NV counter 2 value
          - PASSED

        * - TFM_S_PS_TEST_3008
          - Check PS area version when NV counters 1, 2 and 3 have different values and PS area version match NV counter 3 value
          - PASSED

        * - TFM_S_PS_TEST_3009
          - Check PS area version when NV counter 1 cannot be incremented
          - PASSED

   .. group-tab:: TFM_S_ITS_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_ITS_TEST_1001
          - Set interface
          - PASSED

        * - TFM_S_ITS_TEST_1002
          - Set interface with create flags
          - PASSED

        * - TFM_S_ITS_TEST_1003
          - Set interface with NULL data pointer
          - PASSED

        * - TFM_S_ITS_TEST_1004
          - Set interface with write once UID
          - PASSED

        * - TFM_S_ITS_TEST_1005
          - Get interface with valid data
          - PASSED

        * - TFM_S_ITS_TEST_1006
          - Get interface with zero data length
          - PASSED

        * - TFM_S_ITS_TEST_1007
          - Get interface with invalid UIDs
          - PASSED

        * - TFM_S_ITS_TEST_1008
          - Get interface with data lengths and offsets greater than UID length
          - PASSED

        * - TFM_S_ITS_TEST_1009
          - Get interface with NULL data pointer
          - PASSED

        * - TFM_S_ITS_TEST_1010
          - Get info interface with write once UID
          - PASSED

        * - TFM_S_ITS_TEST_1011
          - Get info interface with valid UID
          - PASSED

        * - TFM_S_ITS_TEST_1012
          - Get info interface with invalid UIDs
          - PASSED

        * - TFM_S_ITS_TEST_1013
          - Remove interface with valid UID
          - PASSED

        * - TFM_S_ITS_TEST_1014
          - Remove interface with write once UID
          - PASSED

        * - TFM_S_ITS_TEST_1015
          - Remove interface with invalid UID
          - PASSED

        * - TFM_S_ITS_TEST_1016
          - Block compaction after remove
          - PASSED

        * - TFM_S_ITS_TEST_1017
          - Multiple partial gets
          - PASSED

        * - TFM_S_ITS_TEST_1018
          - Multiple sets to same UID from same thread
          - PASSED

        * - TFM_S_ITS_TEST_1019
          - Set, get and remove interface with different asset sizes
          - PASSED

        * - TFM_S_ITS_TEST_1023
          - Attempt to get a UID set by a different partition
          - PASSED

   .. group-tab:: TFM_ITS_TEST_2XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_ITS_TEST_2001
          - repetitive sets and gets in/from an asset
          - PASSED

        * - TFM_S_ITS_TEST_2002
          - repetitive sets, gets and removes
          - PASSED

   .. group-tab:: TFM_S_CRYPTO_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_CRYPTO_TEST_1001
          - Secure Key management interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1007
          - Secure Symmetric encryption invalid cipher
          - PASSED

        * - TFM_S_CRYPTO_TEST_1008
          - Secure Symmetric encryption invalid cipher (AES-152)
          - PASSED

        * - TFM_S_CRYPTO_TEST_1010
          - Secure Unsupported Hash (SHA-1) interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1011
          - Secure Hash (SHA-224) interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1012
          - Secure Hash (SHA-256) interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1019
          - Secure Unsupported HMAC (SHA-1) interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1020
          - Secure HMAC (SHA-256) interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1024
          - Secure HMAC with long key (SHA-224) interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1030
          - Secure AEAD (AES-128-CCM) interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1032
          - Secure key policy interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1033
          - Secure key policy check permissions
          - PASSED

        * - TFM_S_CRYPTO_TEST_1034
          - Secure persistent key interface
          - PASSED

        * - TFM_S_CRYPTO_TEST_1035
          - Key access control
          - PASSED

        * - TFM_S_CRYPTO_TEST_1036
          - Secure AEAD interface with truncated auth tag (AES-128-CCM-8)
          - PASSED

        * - TFM_S_CRYPTO_TEST_1037
          - Secure TLS 1.2 PRF key derivation
          - PASSED

        * - TFM_S_CRYPTO_TEST_1038
          - Secure TLS-1.2 PSK-to-MasterSecret key derivation
          - PASSED

        * - TFM_S_CRYPTO_TEST_1040
          - Secure ECDH key agreement
          - PASSED

   .. group-tab:: TFM_S_ATTEST_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_ATTEST_TEST_1004
          - ECDSA signature test of attest token
          - PASSED

        * - TFM_S_ATTEST_TEST_1005
          - Negative test cases for initial attestation service
          - PASSED

   .. group-tab:: TFM_S_PLATFORM_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_S_PLATFORM_TEST_1001
          - Minimal platform service test
          - PASSED


***************
Non-secure test
***************

.. tabs::

   .. group-tab:: Summary

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_IPC_TEST_1XXX
          - IPC non-secure interface test
          - PASSED

        * - TFM_NS_PS_TEST_1XXX
          - PSA protected storage NS interface tests
          - PASSED

        * - TFM_NS_ITS_TEST_1XXX
          - PSA internal trusted storage NS interface tests
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1XXX
          - Crypto non-secure interface test
          - PASSED

        * - TFM_NS_PLATFORM_TEST_1XXX
          - Platform Service Non-Secure interface tests
          - PASSED

        * - TFM_NS_ATTEST_TEST_1XXX
          - Initial Attestation Service non-secure interface tests
          - PASSED

        * - TFM_NS_QCBOR_TEST_1XXX
          - QCBOR regression test
          - PASSED

        * - TFM_NS_T_COSE_TEST_1XXX
          - T_COSE regression test
          - PASSED

   .. group-tab:: TFM_NS_IPC_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_IPC_TEST_1001
          - Get PSA framework version
          - PASSED

        * - TFM_NS_IPC_TEST_1002
          - Get version of an RoT Service
          - PASSED

        * - TFM_NS_IPC_TEST_1003
          - Connect to an RoT Service
          - PASSED

        * - TFM_NS_IPC_TEST_1004
          - Request connection-based RoT Service
          - PASSED

        * - TFM_NS_IPC_TEST_1010
          - Test psa_call with the status of PSA_ERROR_PROGRAMMER_ERROR
          - PASSED

        * - TFM_NS_IPC_TEST_1012
          - Request stateless service
          - PASSED

   .. group-tab:: TFM_NS_PS_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_PS_TEST_1001
          - Set interface
          - PASSED

        * - TFM_NS_PS_TEST_1002
          - Set interface with create flags
          - PASSED

        * - TFM_NS_PS_TEST_1003
          - Set interface with NULL data pointer
          - PASSED

        * - TFM_NS_PS_TEST_1004
          - Set interface with write once UID
          - PASSED

        * - TFM_NS_PS_TEST_1005
          - Get interface with valid data
          - PASSED

        * - TFM_NS_PS_TEST_1006
          - Get interface with zero data length
          - PASSED

        * - TFM_NS_PS_TEST_1007
          - Get interface with invalid UIDs
          - PASSED

        * - TFM_NS_PS_TEST_1008
          - Get interface with invalid data lengths and offsets
          - PASSED

        * - TFM_NS_PS_TEST_1009
          - Get interface with NULL data pointer
          - PASSED

        * - TFM_NS_PS_TEST_1010
          - Get info interface with write once UID
          - PASSED

        * - TFM_NS_PS_TEST_1011
          - Get info interface with valid UID
          - PASSED

        * - TFM_NS_PS_TEST_1012
          - Get info interface with invalid UIDs
          - PASSED

        * - TFM_NS_PS_TEST_1013
          - Remove interface with valid UID
          - PASSED

        * - TFM_NS_PS_TEST_1014
          - Remove interface with write once UID
          - PASSED

        * - TFM_NS_PS_TEST_1015
          - Remove interface with invalid UID
          - PASSED

        * - TFM_NS_PS_TEST_1021
          - Block compaction after remove
          - PASSED

        * - TFM_NS_PS_TEST_1022
          - Multiple partial gets
          - PASSED

        * - TFM_NS_PS_TEST_1023
          - Multiple sets to same UID from same thread
          - PASSED

        * - TFM_NS_PS_TEST_1024
          - Get support interface
          - PASSED

        * - TFM_NS_PS_TEST_1025
          - Set, get and remove interface with different asset sizes
          - PASSED

   .. group-tab:: TFM_NS_ITS_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_ITS_TEST_1001
          - Set interface
          - PASSED

        * - TFM_NS_ITS_TEST_1002
          - Set interface with create flags
          - PASSED

        * - TFM_NS_ITS_TEST_1003
          - Set interface with NULL data pointer
          - PASSED

        * - TFM_NS_ITS_TEST_1004
          - Set interface with write once UID
          - PASSED

        * - TFM_NS_ITS_TEST_1005
          - Get interface with valid data
          - PASSED

        * - TFM_NS_ITS_TEST_1006
          - Get interface with zero data length
          - PASSED

        * - TFM_NS_ITS_TEST_1007
          - Get interface with invalid UIDs
          - PASSED

        * - TFM_NS_ITS_TEST_1008
          - Get interface with invalid data lengths and offsets
          - PASSED

        * - TFM_NS_ITS_TEST_1009
          - Get interface with NULL data pointer
          - PASSED

        * - TFM_NS_ITS_TEST_1010
          - Get info interface with write once UID
          - PASSED

        * - TFM_NS_ITS_TEST_1011
          - Get info interface with valid UID
          - PASSED

        * - TFM_NS_ITS_TEST_1012
          - Get info interface with invalid UIDs
          - PASSED

        * - TFM_NS_ITS_TEST_1013
          - Remove interface with valid UID
          - PASSED

        * - TFM_NS_ITS_TEST_1014
          - Remove interface with write once UID
          - PASSED

        * - TFM_NS_ITS_TEST_1015
          - Remove interface with invalid UID
          - PASSED

        * - TFM_NS_ITS_TEST_1016
          - Block compaction after remove
          - PASSED

        * - TFM_NS_ITS_TEST_1017
          - Multiple partial gets
          - PASSED

        * - TFM_NS_ITS_TEST_1018
          - Multiple sets to same UID from same thread
          - PASSED

        * - TFM_NS_ITS_TEST_1019
          - Set, get and remove interface with different asset sizes
          - PASSED

   .. group-tab:: TFM_NS_CRYPTO_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_CRYPTO_TEST_1001
          - Non Secure Key management interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1007
          - Non Secure Symmetric encryption invalid cipher
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1008
          - Non Secure Symmetric encryption invalid cipher (AES-152)
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1010
          - Non Secure Unsupported Hash (SHA-1) interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1011
          - Non Secure Hash (SHA-224) interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1012
          - Non Secure Hash (SHA-256) interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1019
          - Non Secure Unsupported HMAC (SHA-1) interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1020
          - Non Secure HMAC (SHA-256) interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1024
          - Non Secure HMAC with long key (SHA-224) interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1030
          - Non Secure AEAD (AES-128-CCM) interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1032
          - Non Secure key policy interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1033
          - Non Secure key policy check permissions
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1034
          - Non Secure persistent key interface
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1035
          - Non Secure AEAD interface with truncated auth tag (AES-128-CCM-8)
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1036
          - Non Secure TLS 1.2 PRF key derivation
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1037
          - Non Secure TLS-1.2 PSK-to-MasterSecret key derivation
          - PASSED

        * - TFM_NS_CRYPTO_TEST_1039
          - Non Secure ECDH key agreement
          - PASSED

   .. group-tab:: TFM_NS_PLATFORM_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_PLATFORM_TEST_1001
          - Minimal platform service test
          - PASSED

   .. group-tab:: TFM_NS_ATTEST_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_ATTEST_TEST_1004
          - ECDSA signature test of attest token
          - PASSED

        * - TFM_NS_ATTEST_TEST_1005
          - Negative test cases for initial attestation service
          - PASSED

   .. group-tab:: TFM_NS_QCBOR_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_QCBOR_TEST_1001
          - Regression test of QCBOR library
          - PASSED

   .. group-tab:: TFM_NS_T_COSE_TEST_1XXX

      .. list-table::

        * - **Name**
          - **Descriptions**
          - **Result**

        * - TFM_NS_T_COSE_TEST_1001
          - Regression test of t_cose library
          - PASSED

