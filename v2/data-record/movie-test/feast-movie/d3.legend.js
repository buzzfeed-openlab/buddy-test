// d3.legend.js 
// (C) 2012 ziggy.jonsson.nyc@gmail.com
// MIT licence

// edited (poorly) by christine sunu to include line toggling

(function() {
d3.legend = function(g) {
  g.each(function() {
    var g= d3.select(this),
        items = {},
        svg = d3.select(g.property("nearestViewportElement")),
        legendPadding = g.attr("data-style-padding") || 5,
        lb = g.selectAll(".legend-box").data([true]),
        li = g.selectAll(".legend-items").data([true])

    lb.enter().append("rect").classed("legend-box",true)
    li.enter().append("g").classed("legend-items",true)

    svg.selectAll("[data-legend]").each(function() {
        var self = d3.select(this)
        items[self.attr("data-legend")] = {
          pos : self.attr("data-legend-pos") || this.getBBox().y,
          color : self.attr("data-legend-color") != undefined ? self.attr("data-legend-color") : self.style("fill") != 'none' ? self.style("fill") : self.style("stroke") 
        }
      })

    items = d3.entries(items).sort(function(a,b) { return a.value.pos-b.value.pos})

    li.selectAll("text")
        .data(items,function(d) { return d.key})
        .call(function(d) { d.enter().append("text")})
        .call(function(d) { d.exit().remove()})
        .attr("y",function(d,i) { return i+"em"})
        .attr("x","1em")
        .text(function(d) { ;return d.key})
    
    li.selectAll("circle")
        .data(items,function(d) { return d.key})
        .call(function(d) { d.enter().append("circle")})
        .call(function(d) { d.exit().remove()})
        .attr("cy",function(d,i) { return i-0.25+"em"})
        .attr("cx",0)
        .attr("r","0.4em")
        .style("fill",function(d) { return d.value.color})
        .attr("stroke",function(d) { return d.value.color})
        .style("fill-opacity",function(d) {
            if (selected_keys.indexOf(d.key)!==-1) {
                // not in the index, add it
                return "1.0";
            }
            else {
                return "0.0";
            }})
        .attr("stroke-width","1")
        .on('mouseover', function(d) {
            // console.log('hovering on '+d.key)
            // highlight line
        })
        .on('mouseout',function(d) {
            // console.log('back to normal...')
        })
        .on('click', function(d) {
            // console.log('clicked '+d.key);
            // remove from selected_keys
            if (selected_keys.indexOf(d.key)===-1) {
                // not in the index, add it
                selected_keys.push(d.key);
                d3.select(this).style("fill-opacity","1.0");
            }
            else {
                // in the index, remove it
                selected_keys.splice(selected_keys.indexOf(d.key),1);
                d3.select(this).style("fill-opacity","0.0");
            }
            // dispatch.load(d,selected_keys);
              for (var r=0; r<all_keys.length; r++) {
                if (selected_keys.indexOf(all_keys[r])===-1) {
                    d3.selectAll("rect[id='"+all_keys[r]+"']").style("display","none");
                }
                else {
                    d3.selectAll("rect[id='"+all_keys[r]+"']").style("display",null);
                }
              }
          })
    
    // Reposition and resize the box
    var lbbox = li[0][0].getBBox()  
    lb.attr("x",(lbbox.x-legendPadding))
        .attr("y",(lbbox.y-legendPadding))
        .attr("height",(lbbox.height+2*legendPadding))
        .attr("width",(lbbox.width+2*legendPadding))
  })
  return g
}
})()