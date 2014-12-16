function send_multipart(obj, message, varargin)
    [buffLen, options] = obj.normalize_msg_options(varargin{:});

    if buffLen < 0
        obj.send(message, options{:});
        return
    end

    offset = 1;

    L = length(message);  % length of original message
    N = floor(L/buffLen); % number of multipart messages

    for m = 1:N
        part = message(offset:(offset+buffLen-1));
        offset = offset+buffLen;
        obj.send(part, 'sndmore', options{:});
    end

    part = message(offset:end);
    obj.send(part, options{:});
end