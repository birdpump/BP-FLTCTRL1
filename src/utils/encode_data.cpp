#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>

#include <pb_encode.h>
#include <pb_decode.h>
#include "simple.pb.h"

#include "utils/encode_data.h"

using namespace std;

string encodeData(int number){

        uint8_t buffer[128];
        size_t message_length;
        bool status;

        SimpleMessage message = SimpleMessage_init_zero;

        /* Create a stream that will write to our buffer. */
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        /* Fill in the lucky number */
        message.test_number = number;

        /* Now we are ready to encode the message! */
        status = pb_encode(&stream, SimpleMessage_fields, &message);
        message_length = stream.bytes_written;

        std::stringstream ss;
        pb_ostream_t ostream = pb_ostream_from_buffer(reinterpret_cast<pb_byte_t *>(&ss),
                                                      reinterpret_cast<size_t>(pb_encode_string));
        if (!pb_encode_string(&ostream, buffer, message_length)) {
            std::cerr << "Encoding to string failed" << std::endl;
        }
        std::string encoded_string = ss.str();

        return encoded_string;
}