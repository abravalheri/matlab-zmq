#include <util/socket.h>
#include <util/conversions.h>
#include <util/errors.h>
#include <mex.h>
#include <zmq.h>
#include <string.h>

#define INTERNAL_ABORT_CODE -100

int configure_flag(const mxArray **, int);
void configure_return(int, mxArray **, int, size_t, void *);
size_t configure_buffer_length(const mxArray **, int *);
int recv_with_length(void *, void **, size_t, int);
int recv_without_length(void *, void **, size_t *, int);

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    void *socket = NULL;
    void *buffer = NULL;
    size_t bufLen = 0;
    int optionStart = 1;  /* from which index options can be passed */
    int coreAPIReturn, coreAPIOptionFlag = 0;

    if (nrhs < 1) {
        mexErrMsgIdAndTxt("zmq:core:recv:invalidArgs",
                "Error: At least one argument is required: socket.");
        return;
    }

    if (nrhs >= 2) {
        /* Check if the user has chosen a bufLen,
         * if so get it and update the index where options should start */
        bufLen = configure_buffer_length(prhs, &optionStart);

        /* Get the options for receiving */
        coreAPIOptionFlag = configure_flag(&(prhs[optionStart]), nrhs - optionStart);
    }

    socket = pointer_from_m(prhs[0]);
    if (socket == NULL) {
        mexErrMsgIdAndTxt("zmq:core:recv:invalidSocket", "Error: Invalid socket.");
        return;
    }

    if (bufLen > 0) {
        coreAPIReturn = recv_with_length(socket, &buffer, bufLen, coreAPIOptionFlag);
    } else {
        coreAPIReturn = recv_without_length(socket, &buffer, &bufLen, coreAPIOptionFlag);
    }

    if (coreAPIReturn == INTERNAL_ABORT_CODE) {
        return;
    } else if (coreAPIReturn < 0) {
        handle_error();
    } else {
        /* Prepare the values that should be returned to MATLAB */
        configure_return(nlhs, plhs, coreAPIReturn, bufLen, buffer);
    }

    if (buffer != NULL)
        mxFree(buffer);
}

int recv_with_length(void *socket, void **buffer, size_t bufLen, int coreAPIOptionFlag)
{
    int coreAPIReturn = -1;

    /* Create buffer and call API */
    *buffer = (uint8_t*) mxCalloc(bufLen, sizeof(uint8_t));
    if (*buffer == NULL) {
        mexErrMsgIdAndTxt("util:calloc", "Error: Unsuccessful memory allocation.");
        return INTERNAL_ABORT_CODE;
    }

    coreAPIReturn = zmq_recv(socket, *buffer, bufLen * sizeof(uint8_t), coreAPIOptionFlag);

    return coreAPIReturn;
}

int recv_without_length(void *socket, void **buffer, size_t * bufLen, int coreAPIOptionFlag)
{
    int coreAPIReturn = -1;
    zmq_msg_t message;

    coreAPIReturn = zmq_msg_init(&message);
    if (coreAPIReturn < 0)
        return coreAPIReturn;


    /* Call API and receive message */
    coreAPIReturn = zmq_msg_recv(&message, socket, coreAPIOptionFlag);

    if (coreAPIReturn >= 0) {
        *bufLen = zmq_msg_size(&message);

        if (*bufLen > 0) {
            /* Create buffer */
            *buffer = (uint8_t*) mxCalloc(*bufLen, sizeof(uint8_t));

            if (*buffer == NULL) {
                mexErrMsgIdAndTxt("util:calloc", "Error: Unsuccessful memory allocation.");
                coreAPIReturn = INTERNAL_ABORT_CODE;
            } else {
                memcpy(*buffer, zmq_msg_data(&message), *bufLen);
            }
        }
    }

    zmq_msg_close(&message);

    return coreAPIReturn;
}

/* Discover which options are passed by the user, and calculate the
 * corresponding flag */
int configure_flag(const mxArray **params, int nParams)
{
    int i, coreAPIOptionFlag = 0;
    char *flagStr = NULL;

    for (i = 0 ; i < nParams; i++) {
        flagStr = (char *) str_from_m(params[i]);
        if (flagStr != NULL) {
            if(strcmp(flagStr, "ZMQ_DONTWAIT") == 0)
                coreAPIOptionFlag |= ZMQ_DONTWAIT;
            else
                mexErrMsgIdAndTxt("zmq:core:recv:invalidFlag", "Error: Unknown flag.");
            mxFree(flagStr);
        }
    }

    return coreAPIOptionFlag;
}

/* Check if the message was truncated, advising user and avoiding memory waste
 * while prepare it to return to MATLAB. Also handle the second optional return */
void configure_return(int nlhs, mxArray **plhs, int msgLen, size_t bufLen, void *buffer) {
    if (msgLen > bufLen) {
        mexWarnMsgIdAndTxt("zmq:core:recv:bufferTooSmall",
            "Message is %d bytes long, but buffer is %d. Truncated.",
            msgLen, bufLen);
        plhs[0] = uint8_array_to_m((void*) buffer, bufLen);
    } else {
        plhs[0] = uint8_array_to_m((void*) buffer, msgLen);
    }

    if (nlhs > 1) {
        plhs[1] = int_to_m((void*) &msgLen);
    }
}

/* Check if the param in the index is an number
 * if so, convert it from MATLAB and increments the index */
size_t configure_buffer_length(const mxArray **param, int *paramIndex)
{
    size_t length = 0;
    size_t *input;

    if (mxIsNumeric(param[*paramIndex])) {
        input = (size_t*) size_t_from_m(param[*paramIndex]);
        *paramIndex += 1;
        if (input != NULL) {
            length = *input;
            mxFree(input);
        } else {
            mexErrMsgIdAndTxt("zmq:core:recv:unknowOops", "Unable to convert parameter.");
            return 0;
        }
    }

    return length;
}
