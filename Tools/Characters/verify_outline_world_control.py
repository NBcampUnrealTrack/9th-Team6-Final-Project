"""Compare original native PNG pixels in a static indoor region, without retouching."""
import collections,json,struct,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/NPC003CharacterOutline_20261103'

def png(path):
    data=path.read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n';offset=8;chunks=[]
    while offset<len(data):
        size=struct.unpack_from('>I',data,offset)[0];kind=data[offset+4:offset+8];body=data[offset+8:offset+8+size];offset+=12+size
        if kind==b'IHDR':w,h,bits,color,compression,filtering,interlace=struct.unpack('>IIBBBBB',body)
        elif kind==b'IDAT':chunks.append(body)
    assert bits==8 and color in [2,6] and interlace==0
    channels=3 if color==2 else 4;stride=w*channels;raw=zlib.decompress(b''.join(chunks));previous=bytearray(stride);rows=[]
    assert len(raw)==h*(stride+1)
    for y in range(h):
        start=y*(stride+1);f=raw[start];row=bytearray(raw[start+1:start+1+stride])
        if f==1:
            for x in range(channels,stride):row[x]=(row[x]+row[x-channels])&255
        elif f==2:
            for x in range(stride):row[x]=(row[x]+previous[x])&255
        elif f==3:
            for x in range(stride):row[x]=(row[x]+((row[x-channels] if x>=channels else 0)+previous[x])//2)&255
        elif f==4:
            for x in range(stride):
                a=row[x-channels] if x>=channels else 0;b=previous[x];c=previous[x-channels] if x>=channels else 0;p=a+b-c
                pa,pb,pc=abs(p-a),abs(p-b),abs(p-c);predict=a if pa<=pb and pa<=pc else b if pb<=pc else c
                row[x]=(row[x]+predict)&255
        else:assert f==0,f
        rows.append(row);previous=row
    return w,h,channels,rows

def main():
    folder=OUT/'PreviewCaptures';a=png(folder/'World_Original.png');b=png(folder/'World_Selected.png');assert a[:3]==b[:3]
    counts=collections.Counter();x0,y0,x1,y1=350,180,1200,700
    for y in range(y0,y1):
        for x in range(x0,x1):
            for c in range(3):counts[abs(a[3][y][x*a[2]+c]-b[3][y][x*b[2]+c])]+=1
    n=sum(counts.values());mean=sum(d*number for d,number in counts.items())/n;running=0
    for d,number in sorted(counts.items()):
        running+=number
        if running>=n*.95:p95=d;break
    result={'original':'PreviewCaptures/World_Original.png','selected':'PreviewCaptures/World_Selected.png','region':[x0,y0,x1,y1],
        'character_hidden':True,'mean_absolute_rgb_difference_0_to_255':mean,'p95_absolute_difference':p95,'max_absolute_difference':max(counts),
        'exact_match_fraction':counts[0]/n,'passes_static_world_check':mean<1 and p95<=3}
    (OUT/'WorldVisualCheck.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result));assert result['passes_static_world_check'],'Static world pixels changed more than tolerance'

if __name__=='__main__':main()
