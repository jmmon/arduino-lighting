function main() {
    //      y= 1/(1+e^(((x/4/21)-6)*-1))*255
        //      y= 1/(1+e^(((x/84)-6)*-1))*1024
    let array = [];
    let totalValuesCreated = 1024;  //create this many numbers
    totalValuesCreated += 16;       //adds a buffer which is used to remove some extra 1's and 255's, to cut off some of the values at max/min level.
    for (let i=0; i<totalValuesCreated; i++) {
        array[i] = Math.round(1 / (    1 +  Math.pow( Math.E, ( ( ( (i/(totalValuesCreated/256)) / 21 ) - 6 ) * -1 ) ) ) * 256);

    }
    for (let i=0; i<8; i++) {       //this is what cuts off some extra max/min's
        array.shift();
        array.pop();
    }
    const [w, h] = [32, 32];        //table size
    for (i=0; i<h; i++) {           //print table
        let string = '';
        for (j=0; j<w; j++) {
            string += array[i*w+j];
            string += ", ";
        }
        if (string.match(/^undefined/g) != null) {      //breaks table if size doesn't quite line up
            console.log("Breaking");
            break;
        }
        console.log(`{ ${string}},`);
    }
    //console.log(array.length);
    //console.log(array[285]);

    //console.log("\n", array);
}

main();