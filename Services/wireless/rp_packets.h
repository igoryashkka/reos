


struct rp_payload_CMD_Status {
	uint8_t status;
	uint8_t data;
} __attribute__((__packed__));


struct rp_payload_CMD_Register {
	uint8_t id;
	uint8_t data;
} __attribute__((__packed__));
