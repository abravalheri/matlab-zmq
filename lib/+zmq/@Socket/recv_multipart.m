function message = recv_multipart(obj, varargin)
    [buffLen, options] = obj.normalize_msg_options(varargin{:});

    warning(['buffLen ', num2str(buffLen)]);

    message = [];

    keepReceiving = 1;

    while keepReceiving > 0
        part = obj.recv(buffLen, options{:});
        message = [message part];
        warning(['message ', message]);
        keepReceiving = obj.get('rcvmore');
    end
end