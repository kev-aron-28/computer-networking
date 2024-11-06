Sender:
- Keeps track of unacknowledged packets in a sliding window.
- Sends packets in the window, waits for ACKs, and retransmits on timeout Receiver:
Only accepts packets in order.
- Sends an ACK for the last correctly received packet.
