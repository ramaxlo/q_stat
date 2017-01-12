#!/usr/bin/env python

import os, sys
import subprocess

docker_image = 'q_stat'

def usage():
	print 'Usage: q_stat.py <video file>'

def run_docker(f):
	if not f.startswith('/'):
		f = os.path.join(os.getcwd(), f)

	cmd = ['sudo', 'docker', 'run', '-t', '--rm', '-v']
	volume = '%s:/root/%s:ro' % (f, os.path.basename(f))
	cmd.append(volume)
	cmd.append('q_stat')
	cmd.append(os.path.basename(f))

	rc = subprocess.call(cmd)

	if rc != 0:
		return False

	return True

if __name__ == '__main__':
	if len(sys.argv) < 2:
		usage()
		sys.exit(1)

	video = sys.argv[1]
	if not run_docker(video):
		sys.exit(1)
