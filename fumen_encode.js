
// Who needs "var"?
baseurl="http://fumen.zui.jp/";
enclim=32768; // 限界エンコード文字数
enc=new Array(enclim+1024); // エンコード配列 (Encode array)
enctbl='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'; // エンコード文字テーブル
asctbl=' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'; // ASCII文字テーブル
framelim=2000; // 限界フレーム数
fldlines=24; // フィールド段数
fldblks=fldlines*10; // フィールドブロック数
f=new Array(fldblks);encf=new Array(fldblks);af=new Array(fldblks*(framelim+1)); // 仮想フィールド (Playfield)
p=new Array(3);
ap=new Array(3*(framelim+1)); // ピース種類,角度,座標 (Piece placements)
au=new Array(framelim); // せり上がり (Rise)
am=new Array(framelim); // 反転スイッチ (Mirror)
ac=new Array(framelim); // コメント (Comment)
ad=new Array(framelim); // 接着 (Lock)
ct=0; // 色 (Color, 0 = ARS, 1 = SRS)
b=new Array( // ピースパターン (Piece Pattern)
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,1,1,2,1,3,1,1,0,1,1,1,2,1,3,0,1,1,1,2,1,3,1,1,0,1,1,1,2,1,3,
0,1,1,1,2,1,0,2,1,0,1,1,1,2,2,2,2,0,0,1,1,1,2,1,0,0,1,0,1,1,1,2,
1,1,2,1,1,2,2,2,1,1,2,1,1,2,2,2,1,1,2,1,1,2,2,2,1,1,2,1,1,2,2,2,
0,1,1,1,1,2,2,2,2,0,1,1,2,1,1,2,0,1,1,1,1,2,2,2,2,0,1,1,2,1,1,2,
0,1,1,1,2,1,1,2,1,0,1,1,2,1,1,2,1,0,0,1,1,1,2,1,1,0,0,1,1,1,1,2,
0,1,1,1,2,1,2,2,1,0,2,0,1,1,1,2,0,0,0,1,1,1,2,1,1,0,1,1,0,2,1,2,
1,1,2,1,0,2,1,2,0,0,0,1,1,1,1,2,1,1,2,1,0,2,1,2,0,0,0,1,1,1,1,2
);

framemax=0; // 最大フレーム番号
frame=0; // フレーム番号

function newdata()
{
    frame = framemax = 0;

    // Reset field data
    for(i=0;i<fldblks;i++)af[frame*fldblks+i]=f[i]=0;

    ap[frame*3+0]=0;
    ap[frame*3+1]=0;
    ap[frame*3+2]=0;
    au[frame]=0;
    am[frame]=0;
    ac[frame]='';
    ad[frame]=0;
    ct=0;
}

function pushframe(pfframe, comment) // 指定フレームへ転送
{
    // Copy field
    for(pfi=0;pfi<fldblks;pfi++)af[pfframe*fldblks+pfi]=f[pfi];

    // Copy piece placement
    for(pfi=0;pfi<3;pfi++)ap[pfframe*3+pfi]=p[pfi];

    au[pfframe]=0;
    am[pfframe]=0;
    ac[pfframe]=comment;
    ad[pfframe]=0;
}

function processframe(rise, mirror){
    // ブロック配置 (Place blocks on field)
    if(p[0]>0){
        for(i=0;i<4;i++)f[p[2]+b[p[0]*32+p[1]*8+i*2+1]*10+b[p[0]*32+p[1]*8+i*2]-11]=p[0];
        p[0]=0;p[1]=0;p[2]=0;
    }

    // フィールドずらし消去 (Clear completed lines)
    for(i=fldlines-2,k=fldlines-2;k>=0;k--){
        chk=0;for(j=0;j<10;j++)chk+=(f[k*10+j]>0);
        if(chk<10){
            for(j=0;j<10;j++)f[i*10+j]=f[k*10+j];
            i--;
        }
    }
    for(;i>=0;i--)for(j=0;j<10;j++)f[i*10+j]=0;

    // 直後にせり上がり
    if(rise){for(i=0;i<fldblks-10;i++)f[i]=f[i+10];for(i=fldblks-10;i<fldblks;i++)f[i]=0;}
    // 直後にミラー発動
    if(mirror)for(i=0;i<fldlines-1;i++)for(j=0;j<5;j++){tmp=f[i*10+j];f[i*10+j]=f[i*10+9-j];f[i*10+9-j]=tmp;}
}

function encodeFumen()
{
    pushframe(frame, "end");
    cmstrrep='';
    encc=0;
    fldrepaddr=-1;
    for(i=0;i<fldblks;i++)encf[i]=0;
    for(e=0;encc<enclim&&e<=framemax;e++){
        // フィールド出力 (Field output)
        for(j=0;j<fldblks;j++)encf[j]=af[e*fldblks+j]+8-encf[j];
        fc=0;
        for(j=0;j<fldblks-1;j++){
            fc++;
            if(encf[j]!=encf[j+1]){
                tmp=encf[j]*fldblks+(fc-1);
                enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
                enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
                fc=0;
            }
        }
        fc++;
        tmp=encf[j]*fldblks+(fc-1);
        enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
        enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
        fc=0;
        if(enc[encc-2]+enc[encc-1]*64==9*fldblks-1){
            if(fldrepaddr<0){
                fldrepaddr=encc++;enc[fldrepaddr]=0;
            }else{
                if(enc[fldrepaddr]<63){enc[fldrepaddr]++;encc-=2;}else{fldrepaddr=encc++;enc[fldrepaddr]=0;}
            }
        }else{
            if(fldrepaddr>=0)fldrepaddr=-1;
        }
        // タイプ,角度,座標出力
        cmstrrep=(e>0)?ac[e-1]:'';
        // refreshquiz(e,1,0,1,0);
        cmstrrep=escape(cmstrrep).substring(0,4095);
        tmpstr=escape(ac[e]).substring(0,4095);
        tmplen=tmpstr.length;
        if(tmplen>4095)tmplen=4095;
        tmp=ap[e*3+0]+8*(ap[e*3+1]+4*(ap[e*3+2]+fldblks*(au[e]+2*(am[e]+2*((e==0)*ct+2*((tmpstr!=cmstrrep)+2*ad[e]))))));
        enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
        enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
        enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
        // コメント出力 (Comment output)
        if(tmpstr!=cmstrrep){
            tmp=tmplen;
            enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
            enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
            for(i=0;i<tmplen;i+=4){
                tmp=(((ins=asctbl.indexOf(tmpstr.charAt(i+0)))>=0?ins:0)%96);
                tmp+=(((ins=asctbl.indexOf(tmpstr.charAt(i+1)))>=0?ins:0)%96)*96;
                tmp+=(((ins=asctbl.indexOf(tmpstr.charAt(i+2)))>=0?ins:0)%96)*9216;
                tmp+=(((ins=asctbl.indexOf(tmpstr.charAt(i+3)))>=0?ins:0)%96)*884736;
                enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
                enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
                enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
                enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
                enc[encc++]=tmp%64;tmp=Math.floor(tmp/64);
            }
        }
        // フィールド記憶 (Encode field)
        for(j=0;j<fldblks;j++)encf[j]=af[e*fldblks+j];
        if(!ad[e]){ // ミノ接着 (Mino Lock)
            // ブロック配置 (Place blocks on field)
            if(ap[e*3+0]>0){
                for(j=0;j<4;j++)encf[ap[e*3+2]+b[ap[e*3+0]*32+ap[e*3+1]*8+j*2+1]*10+b[ap[e*3+0]*32+ap[e*3+1]*8+j*2]-11]=ap[e*3+0];
            }
            // フィールドずらし消去 (Clear completed lines)
            for(i=fldlines-2,k=fldlines-2;k>=0;k--){
                chk=0;for(j=0;j<10;j++)chk+=(encf[k*10+j]>0);
                if(chk<10){
                    for(j=0;j<10;j++)encf[i*10+j]=encf[k*10+j];
                    i--;
                }
            }
            for(;i>=0;i--)for(j=0;j<10;j++)encf[i*10+j]=0;
            // せり上がり (Rise)
            if(au[e]){for(i=0;i<fldblks-10;i++)encf[i]=encf[i+10];for(i=fldblks-10;i<fldblks;i++)encf[i]=0;}
            // ミラー発動 (Mirror)
            if(am[e])for(i=0;i<fldlines-1;i++)for(j=0;j<5;j++){tmp=encf[i*10+j];encf[i*10+j]=encf[i*10+9-j];encf[i*10+9-j]=tmp;}
        }
    }
    encstr='v115@';
    for(i=0;i<encc;i++){
        encstr=encstr+enctbl.charAt(enc[i]);
        if(i%47==41)encstr=encstr+"?";
    }

    return encstr;
}

function pad(n, width, z)
{
    z = z || '0';
    n = n + '';
    return n.length >= width ? n : new Array(width - n.length + 1).join(z) + n;
}

function convertFrameTime(time)
{
    time /= 60;
    var ms = Math.floor((time * 100) % 100);
    var m  = Math.floor(time / 60);
    var s  = Math.floor(time - (m * 60));

    return pad(m, 2) + ":" + pad(s, 2) + ":" + pad(ms, 2);
}

function encode()
{
    newdata();

    // Figure out which mode is selected
    var mode = document.getElementsByName("tap-mode");
    for (var i = 0, len = mode.length; i < len; ++i)
    {
        if (mode[i].checked)
        {
            mode = mode[i].value;
            break;
        }
    }

    var rollReset = false;
    var currentSection = 0;
    var level = 0;
    var timer = 0;

    var fumenData = document.getElementById("pdata-text").value.split("\n");

    for (i = 0, len = fumenData.length; i < len; ++i)
    {
        var line = fumenData[i];
        if (line.length === 0 || !line.trim())
        {
            console.log("skip");
            continue;
        }

        var data = line.split(",");

        if (data.length < 7)
        {
            console.log("not enough elements");
            continue;
        }

        var grade = data[0];
        level  = parseInt(data[1]);
        timer  = parseInt(data[2]);
        var piece  = parseInt(data[3]);
        var xcoord = parseInt(data[4]);
        var ycoord = parseInt(data[5]);
        var rot    = parseInt(data[6]);
        var mroll  = parseInt(data[7]);

        var inCreditRoll = data.length > 8 ? parseInt(data[8]) : false;

        if (inCreditRoll && !rollReset)
        {
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;

            pushframe(frame, "Credit Roll!");
            processframe(0, 0);
            frame++;

            // Reset the field in master mode.
            if (mode === "master")
            {
                for(e=0;e<fldblks;e++) f[e]=0;
            }

            rollReset = true;
        }

        p[0] = piece;
        p[1] = rot;
        p[2] = 240 - ycoord * 10 + xcoord;

        var comment = "";
        if (mode === "master")
        {
            comment += mroll ? '~' : '';
            comment += grade;
        }
        if (document.getElementById("comment-section").checked)
        {
            if (level < 999)
            {
                if (level > (currentSection + 1) * 100)
                {
                    currentSection++;
                }
                comment += " (" + currentSection * 100 + " - " + ((currentSection + 1) * 100 - 1) + ")";
            }
        }
        if (document.getElementById("comment-level").checked)
        {
            comment += " " + level;
        }
        if (document.getElementById("comment-time").checked)
        {
            comment += " @ " + convertFrameTime(timer);
        }

        pushframe(frame, comment);
        processframe(0, 0);
        frame++;
    }
    framemax = frame;

    var fumenStr = encodeFumen();
    var fumenURL = baseurl + "?" + fumenStr + "#english.js";
    var fumenList = document.getElementById("fumen-list");

    var li = document.createElement("li");
    var rawLink = document.createElement("a");
    rawLink.href = fumenURL;
    rawLink.innerHTML = level + " @ " + convertFrameTime(timer) + " (" + fumenURL.length + ")";

    var injectLink = document.createElement("a");
    injectLink.href = "javascript:(function(){document.getElementById(\"tx\").value=\"" + fumenStr + "\";versioncheck(0);})()";
    injectLink.innerHTML = " [b] ";

    li.appendChild(rawLink);
    li.appendChild(injectLink);
    fumenList.insertBefore(li, fumenList.childNodes[0]);
}

function uploadFile(e)
{
    var file = e.target.files[0];

    var reader = new FileReader();
    reader.onload = function(e)
    {
        document.getElementById("pdata-text").value = e.target.result;
    };

    reader.readAsText(file);
}

window.onload = function()
{
    var upload = document.getElementById("pdata-upload");
    upload.addEventListener('change', uploadFile, false);

    var encodeButton = document.getElementById("encode");
    encodeButton.addEventListener('click', encode, false);
};
