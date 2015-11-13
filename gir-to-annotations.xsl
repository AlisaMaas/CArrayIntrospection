<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:c="http://www.gtk.org/introspection/c/1.0" xmlns:gir="http://www.gtk.org/introspection/core/1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>
	<xsl:strip-space elements="*"/>

	<xsl:template match="/gir:repository">
		<xsl:text>{"library_functions":[</xsl:text>
		<xsl:apply-templates select="//gir:function | //gir:method"/>
  		<xsl:text>]}</xsl:text>
	</xsl:template>

	<xsl:template match="gir:function | gir:method">
		<xsl:if test="position() != 1">,</xsl:if>
		<xsl:text>{"function_name":"</xsl:text>
		<xsl:value-of select="@c:identifier"/>
		<xsl:text>","arguments":[</xsl:text>
		<xsl:apply-templates select="gir:parameters"/>
		<xsl:text>]}</xsl:text>
	</xsl:template>

	<xsl:template match="gir:parameter | gir:instance-parameter">
		<xsl:if test="position() != 1">,</xsl:if>
		<xsl:text>{"argument_name":"</xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>"</xsl:text>
		<xsl:apply-templates select="gir:array | gir:type"/>
		<xsl:text>}</xsl:text>
	</xsl:template>

	<xsl:template match="gir:array">
		<xsl:apply-templates select="@fixed-size | @length | @zero-terminated"/>
	</xsl:template>

	<xsl:template match="@fixed-size">
		<xsl:text>,"fixed":</xsl:text>
		<xsl:value-of select="."/>
	</xsl:template>

	<xsl:template match="@length">
		<xsl:text>,"symbolic":</xsl:text>
		<xsl:variable name="instance-offset" select="count(ancestor::gir:parameter[1]/preceding-sibling::gir:instance-parameter)"/>
		<xsl:value-of select=". + $instance-offset"/>
	</xsl:template>

	<xsl:template match="@zero-terminated[. = '0']"/>

	<xsl:template match="@zero-terminated">
		<xsl:text>,"sentinel":"NUL"</xsl:text>
	</xsl:template>

	<xsl:template match="gir:type[@name = 'String' or @name = 'utf8']">
		<xsl:text>,"sentinel":"NUL"</xsl:text>
	</xsl:template>
</xsl:stylesheet>

<!--
    Local variables:
    nxml-child-indent: 8
    End:
-->
