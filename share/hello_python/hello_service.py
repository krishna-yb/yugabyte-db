#!/usr/bin/env python3
"""
Simple Python service that prints messages periodically.
Demonstrates ProcessWrapper integration with TServer.
"""

import time
import signal
import sys
import os
from datetime import datetime

# Force unbuffered output so prints show up immediately in logs
sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', buffering=1)
sys.stderr = os.fdopen(sys.stderr.fileno(), 'w', buffering=1)

class HelloService:
    def __init__(self, message="Hello from Python!", interval=5):
        self.message = message
        self.interval = interval
        self.running = True
        self.counter = 0
        
    def handle_signal(self, signum, frame):
        """Handle signals from TServer"""
        if signum == signal.SIGTERM:
            print(f"[{self.get_timestamp()}] Received SIGTERM, shutting down gracefully...")
            self.running = False
        elif signum == signal.SIGHUP:
            print(f"[{self.get_timestamp()}] Received SIGHUP, reloading config...")
            # Reload message from environment variable
            self.message = os.getenv('HELLO_MESSAGE', self.message)
            print(f"[{self.get_timestamp()}] New message: {self.message}")
    
    def get_timestamp(self):
        """Get current timestamp"""
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    def run(self):
        """Main service loop"""
        print(f"[{self.get_timestamp()}] ========================================")
        print(f"[{self.get_timestamp()}] Hello Service starting...")
        print(f"[{self.get_timestamp()}] PID: {os.getpid()}")
        print(f"[{self.get_timestamp()}] Python: {sys.executable}")
        print(f"[{self.get_timestamp()}] Python Version: {sys.version.split()[0]}")
        venv_path = os.getenv('VIRTUAL_ENV', 'Not in venv')
        print(f"[{self.get_timestamp()}] Virtual Env: {venv_path}")
        print(f"[{self.get_timestamp()}] Message: {self.message}")
        print(f"[{self.get_timestamp()}] Interval: {self.interval}s")
        print(f"[{self.get_timestamp()}] ========================================")
        
        while self.running:
            self.counter += 1
            print(f"[{self.get_timestamp()}] [{self.counter}] {self.message}")
            sys.stdout.flush()  # Force output to appear immediately
            time.sleep(self.interval)
        
        print(f"[{self.get_timestamp()}] Hello Service stopped after {self.counter} iterations")

def main():
    """Entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Hello Python Service')
    parser.add_argument('--message', type=str, default='Hello from Python!',
                        help='Message to print')
    parser.add_argument('--interval', type=int, default=5,
                        help='Interval between messages (seconds)')
    
    args = parser.parse_args()
    
    # Override from environment if set
    message = os.getenv('HELLO_MESSAGE', args.message)
    interval = int(os.getenv('HELLO_INTERVAL', args.interval))
    
    # Create service
    service = HelloService(message=message, interval=interval)
    
    # Register signal handlers
    signal.signal(signal.SIGTERM, service.handle_signal)
    signal.signal(signal.SIGHUP, service.handle_signal)
    
    # Run
    try:
        service.run()
    except KeyboardInterrupt:
        print(f"\n[{service.get_timestamp()}] Interrupted by user")
    except Exception as e:
        print(f"[{service.get_timestamp()}] Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
