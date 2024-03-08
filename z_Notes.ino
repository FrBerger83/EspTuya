/*

a lot of info from       https://github.com/jasonacox/tinytuya
on Tuya communications   https://github.com/jasonacox/tinytuya/issues/28
on tuya protocol         https://github.com/jasonacox/tinytuya/discussions/260
on kwh                   https://github.com/jasonacox/tinytuya/discussions/239
on cloud                 https://github.com/jasonacox/tinytuya/discussions/79

Hex decoder         https://cryptii.com/pipes/hex-decoder
Test 55AA msg       http://aes.online-domain-tools.com/ to test 55AA messages
Test CRC 55AA msg   https://www.liavaag.org/English/SHA-Generator/HMAC/ 


client persitent https://arduino.stackexchange.com/questions/65604/creating-http-persistent-connection-to-create-smooth-data-over-gsm-sending


Protocol commands https://github.com/codetheweb/tuyapi/blob/master/lib/message-parser.js  



From Tinytuya :

As a recap, the older 55AA/v3.1-v3.4 packet format looks like:

000055aaSSSSSSSSMMMMMMMMLLLLLLLL[RRRRRRRR]DD..DDCC..CC0000aa55 where:
000055aa - prefix
SSSSSSSS - 32-bit sequence number
MMMMMMMM - 32-bit Command ID
LLLLLLLL - 32-bit packet length - count every byte from the return code through (and including) the footer
[RRRRRRRR] - packets from devices have a 32-bit return code. Packets from the app/client do not have this field
DD..DD - variable length encrypted payload data
CC..CC - checksum, either 32-bit (4-byte) CRC32 for v3.1-v3.3, or 256-bit (32-byte) HMAC-SHA256 for v3.4
0000aa55 - footer

Everything except the payload data is sent unencrypted/in the clear. Payload data is padded to have a multiple-of-16 length and is AES128 encrypted in ECB mode. All multi-byte integers are big-endian.

v3.5 format looks like:

00006699UUUUSSSSSSSSMMMMMMMMLLLLLLLL(II*12)DD..DD(TT*16)00009966
00006699 - prefix
UUUU - Unknown 16-bit or 2*8-bit field. Current devices always transmit as 0x0000 and completely ignore on reception
SSSSSSSS - 32-bit sequence number
MMMMMMMM - 32-bit Command ID
LLLLLLLL - 32-bit packet length - count every byte from the IV/nonce through (and including) the Tag, but not the footer
(II*12) - 96-bit (12-byte) per-packet GCM IV/nonce
DD..DD - variable length encrypted payload data
(TT*16) - 128-bit (16-byte) signature Tag value from AES-GCM encrypt-and-sign
00009966 - footer








*/