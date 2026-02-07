-- Copyright (C) 2010 <SWGEmu>

object_tangible_terminal_terminal_imagedesign = object_tangible_terminal_shared_terminal_imagedesign:new {
	objectMenuComponent = "ImageDesignTerminalMenuComponent",
    dataObjectComponent = "ImageDesignTerminalDataComponent",
	planetMapCategory = "terminal",
	maxCondition = 0,
	planetMapSubCategory = "terminal_imagedesign"
}

ObjectTemplates:addTemplate(object_tangible_terminal_terminal_imagedesign, "object/tangible/terminal/terminal_imagedesign.iff")
