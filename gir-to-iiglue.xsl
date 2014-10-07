<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:c="http://www.gtk.org/introspection/c/1.0" xmlns:gir="http://www.gtk.org/introspection/core/1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>
	<xsl:strip-space elements="*"/>
	<xsl:template match="/gir:repository">
		<xsl:text>{"libraryFunctions":[</xsl:text>
		<xsl:for-each select="//gir:function/gir:parameters | //gir:method/gir:parameters">
			<xsl:if test="position() != 1">,</xsl:if>
			<xsl:text>{"foreignFunctionName":"</xsl:text>
			<xsl:value-of select="../@c:identifier"/>
			<xsl:text>","foreignFunctionParameters":[</xsl:text>
			<xsl:for-each select="gir:parameter | gir:instance-parameter">
				<xsl:if test="position() != 1">,</xsl:if>
				<xsl:text>{"parameterAnnotations":[</xsl:text>
				<xsl:if test="gir:type[@name='utf8' or @name='String'] | gir:array">
					<xsl:text>{"PAArray":1}</xsl:text>
				</xsl:if>
				<xsl:text>]}</xsl:text>
			</xsl:for-each>
		<xsl:text>]}</xsl:text>
		</xsl:for-each>
  	<xsl:text>]}</xsl:text>
	</xsl:template>
</xsl:stylesheet>