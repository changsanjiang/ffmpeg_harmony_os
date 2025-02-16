/**
 This file is part of @sj/ffmpeg.

 @sj/ffmpeg is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 @sj/ffmpeg is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with @sj/ffmpeg. If not, see <http://www.gnu.org/licenses/>.
 * */
namespace FFProgressMessageParser {
  // frame=61
  // fps=0.00
  // stream_0_0_q=1.6
  // bitrate=   0.1kbits/s
  // total_size=44
  // out_time_us=2688000
  // out_time_ms=2688000
  // out_time=00:00:02.688000
  // dup_frames=0
  // drop_frames=0
  // speed=4.98x
  // progress=continue
  export interface Data {
    frame: number;
    fps: number;
    stream_q: number;
    bitrate: number;
    total_size: number;
    time_us: number;
    time_ms: number;
    out_time: string;
    dup_frames: number;
    drop_frames: number;
    speed: number;
    progress: string;
  }

  export function parse(progressMessage: string): Data {
    const lines = progressMessage.split('\n');
    const data: Data = {
      frame: 0,
      fps: 0,
      stream_q: 0,
      bitrate: 0,
      total_size: 0,
      time_us: 0,
      time_ms: 0,
      out_time: '',
      dup_frames: 0,
      drop_frames: 0,
      speed: 0,
      progress: ''
    };

    for (const line of lines) {
      const parsed = parseLine(line);
      if (parsed) {
        const key = parsed[0];
        const value = parsed[1];
        switch (key) {
          case 'frame':
            data.frame = parseInt(value, 10);
            break;
          case 'fps':
            data.fps = parseFloat(value);
            break;
          case 'stream_0_0_q':
            data.stream_q = parseFloat(value);
            break;
          case 'bitrate':
            data.bitrate = parseFloat(value) * 1000;
            break;
          case 'total_size':
            data.total_size = parseInt(value, 10);
            break;
          case 'out_time_us':
            data.time_us = parseInt(value, 10);
            break;
          case 'out_time_ms':
            data.time_ms = parseInt(value, 10);
            break;
          case 'out_time':
            data.out_time = value.trim();
            break;
          case 'dup_frames':
            data.dup_frames = parseInt(value, 10);
            break;
          case 'drop_frames':
            data.drop_frames = parseInt(value, 10);
            break;
          case 'speed':
            data.speed = parseFloat(value);
            break;
          case 'progress':
            data.progress = value.trim();
            break;
        }
      }
    }
    return data;
  }

  function parseLine(line: string): [string, string] | undefined {
    const regex = /^\s*(\w+)=([^=]+)\s*$/;
    const match = line.match(regex);
    return match ? [match[1], match[2]] : undefined;
  }
}

export default FFProgressMessageParser;