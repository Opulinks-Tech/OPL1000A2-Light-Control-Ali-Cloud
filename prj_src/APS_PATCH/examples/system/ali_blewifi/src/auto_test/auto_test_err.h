#ifndef _AUTO_TEST_ERR_H_
#define _AUTO_TEST_ERR_H_

#define ATS_OK                 0
#define ATS_SOCK_READ_FAILED   0x100
#define ATS_SOCK_READ_TIMEOUT  0x101
#define ATS_SOCK_READ_CLOSE    0x102
#define ATS_SOCK_WRITE_FAILED  0x103
#define ATS_SOCK_WRITE_TIMEOUT 0x104
#define ATS_SOCK_WRITE_CLOSE   0x105
#define ATS_SOCK_CONNECT_FAIL  0x106
#define ATS_DNS_FAIL           0x107
#define ATS_ALI_REPORT_TOKEN_FAILED  0x200

#define ATS_POST_OVERWRITTEN    0x300
#define ATS_POST_CLEARED        0x301

#endif /* _AUTO_TEST_ERR_H_ */
